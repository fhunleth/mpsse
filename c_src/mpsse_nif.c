/*
 *  Copyright 2023 Frank Hunleth
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * MPSSE NIF implementation.
 */

#include <erl_nif.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "libmpsse/src/mpsse.h"

//#define DEBUG

#ifdef DEBUG
#define log_location stderr
//#define LOG_PATH "/tmp/mpsse.log"
#define debug(...) do { enif_fprintf(log_location, __VA_ARGS__); enif_fprintf(log_location, "\r\n"); fflush(log_location); } while(0)
#define error(...) do { debug(__VA_ARGS__); } while (0)
#define start_timing() ErlNifTime __start = enif_monotonic_time(ERL_NIF_USEC)
#define elapsed_microseconds() (enif_monotonic_time(ERL_NIF_USEC) - __start)
#else
#define debug(...)
#define error(...) do { enif_fprintf(stderr, __VA_ARGS__); enif_fprintf(stderr, "\n"); } while(0)
#define start_timing()
#define elapsed_microseconds() 0
#endif

// MPSSE NIF Resource.
struct MPSSENifRes {
    struct mpsse_context *mpsse;
};

// MPSSE NIF Private data
struct MPSSENifPriv {
    ErlNifResourceType *mpsse_nif_res_type;
};

static ERL_NIF_TERM atom_ok;
static ERL_NIF_TERM atom_error;

static int mpsse_open_count = 0;

static void mpsse_dtor(ErlNifEnv *env, void *obj)
{
    struct MPSSENifRes *res = (struct MPSSENifRes *) obj;

    debug("mpsse_dtor");
    if (res->mpsse) {
        Close(res->mpsse);
        res->mpsse = NULL;
    }
}

static int mpsse_load(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM info)
{
#ifdef DEBUG
#ifdef LOG_PATH
    log_location = fopen(LOG_PATH, "w");
#endif
#endif
    debug("mpsse_load");

    struct MPSSENifPriv *priv = enif_alloc(sizeof(struct MPSSENifPriv));
    if (!priv) {
        error("Can't allocate MPSSE priv");
        return 1;
    }

    priv->mpsse_nif_res_type = enif_open_resource_type(env, NULL, "mpsse_nif_res_type", mpsse_dtor, ERL_NIF_RT_CREATE, NULL);
    if (priv->mpsse_nif_res_type == NULL) {
        error("open MPSSE NIF resource type failed");
        return 1;
    }

    atom_ok = enif_make_atom(env, "ok");
    atom_error = enif_make_atom(env, "error");

    *priv_data = priv;
    return 0;
}

static void mpsse_unload(ErlNifEnv *env, void *priv_data)
{
    debug("mpsse_unload");
    struct MPSSENifPriv *priv = enif_priv_data(env);

    enif_free(priv);
}

static ERL_NIF_TERM mpsse_open(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    struct MPSSENifPriv *priv = enif_priv_data(env);
    int mode, freq, endianness;

    if (!enif_get_int(env, argv[0], &mode) ||
        !enif_get_int(env, argv[1], &freq) ||
        !enif_get_int(env, argv[2], &endianness))
        return enif_make_badarg(env);

    struct mpsse_context *mpsse = MPSSE((enum modes) mode, freq, endianness);
    if (!mpsse)
        return atom_error;

    struct MPSSENifRes *mpsse_nif_res = enif_alloc_resource(priv->mpsse_nif_res_type, sizeof(struct MPSSENifRes));
    mpsse_nif_res->mpsse = mpsse;
    ERL_NIF_TERM res_term = enif_make_resource(env, mpsse_nif_res);

    mpsse_open_count++;

    // Elixir side owns the resource. Safe for NIF side to release it.
    enif_release_resource(mpsse_nif_res);

    return enif_make_tuple2(env, atom_ok, res_term);
}

static ERL_NIF_TERM mpsse_close(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    struct MPSSENifPriv *priv = enif_priv_data(env);
    struct MPSSENifRes *res;

    if (!enif_get_resource(env, argv[0], priv->mpsse_nif_res_type, (void **)&res))
        return enif_make_badarg(env);

    if (res->mpsse) {
        Close(res->mpsse);
        res->mpsse = NULL;
        mpsse_open_count--;
    }

    return atom_ok;
}

static ERL_NIF_TERM mpsse_start(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    struct MPSSENifPriv *priv = enif_priv_data(env);
    struct MPSSENifRes *res;

    if (!enif_get_resource(env, argv[0], priv->mpsse_nif_res_type, (void **)&res))
        return enif_make_badarg(env);

    if (Start(res->mpsse) == MPSSE_OK)
        return atom_ok;
    else
        return atom_error;
}

static ERL_NIF_TERM mpsse_stop(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    struct MPSSENifPriv *priv = enif_priv_data(env);
    struct MPSSENifRes *res;

    if (!enif_get_resource(env, argv[0], priv->mpsse_nif_res_type, (void **)&res))
        return enif_make_badarg(env);

    if (Stop(res->mpsse) == MPSSE_OK)
        return atom_ok;
    else
        return atom_error;
}

static ERL_NIF_TERM mpsse_read(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    struct MPSSENifPriv *priv = enif_priv_data(env);
    struct MPSSENifRes *res;
    unsigned long read_len;
    ERL_NIF_TERM bin_read;
    unsigned char *raw_bin_read;

    if (!enif_get_resource(env, argv[0], priv->mpsse_nif_res_type, (void **)&res) ||
        !enif_get_ulong(env, argv[1], &read_len))
        return enif_make_badarg(env);

    raw_bin_read = enif_make_new_binary(env, read_len, &bin_read);

    if (!raw_bin_read)
        return enif_make_tuple2(env, atom_error, enif_make_atom(env, "alloc_failed"));

    char *result = Read(res->mpsse, read_len);
    if (result) {
        memcpy(raw_bin_read, result, read_len);
        free(result);
        return enif_make_tuple2(env, atom_ok, bin_read);
    } else
        return atom_error;
}

static ERL_NIF_TERM mpsse_write(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    struct MPSSENifPriv *priv = enif_priv_data(env);
    struct MPSSENifRes *res;
    ErlNifBinary bin_write;

    if (!enif_get_resource(env, argv[0], priv->mpsse_nif_res_type, (void **)&res) ||
        !enif_inspect_iolist_as_binary(env, argv[1], &bin_write))
        return enif_make_badarg(env);

    if (Write(res->mpsse, (const char *) bin_write.data, bin_write.size) == MPSSE_OK)
        return atom_ok;
    else
        return atom_error;
}

static ERL_NIF_TERM mpsse_get_ack(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
    struct MPSSENifPriv *priv = enif_priv_data(env);
    struct MPSSENifRes *res;

    if (!enif_get_resource(env, argv[0], priv->mpsse_nif_res_type, (void **)&res))
        return enif_make_badarg(env);

    return enif_make_int(env, GetAck(res->mpsse));
}

static ErlNifFunc nif_funcs[] =
{
    {"open", 3, mpsse_open, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"close", 1, mpsse_close, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"start", 1, mpsse_start, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"stop", 1, mpsse_stop, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"write", 2, mpsse_write, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"get_ack", 1, mpsse_get_ack, ERL_NIF_DIRTY_JOB_IO_BOUND},
    {"read", 2, mpsse_read, ERL_NIF_DIRTY_JOB_IO_BOUND},
};

ERL_NIF_INIT(Elixir.MPSSE.NIF, nif_funcs, mpsse_load, NULL, NULL, mpsse_unload)
