defmodule MPSSE.NIF do
  @on_load {:load_nif, 0}
  @compile {:autoload, false}

  @moduledoc false

  def load_nif() do
    nif_binary = Application.app_dir(:mpsse, "priv/mpsse_nif")
    :erlang.load_nif(to_charlist(nif_binary), 0)
  end

  def open(_mode, _freq, _endianness), do: :erlang.nif_error(:nif_not_loaded)
  def close(_ref), do: :erlang.nif_error(:nif_not_loaded)

  def start(_ref), do: :erlang.nif_error(:nif_not_loaded)
  def stop(_ref), do: :erlang.nif_error(:nif_not_loaded)
  def write(_ref, _data), do: :erlang.nif_error(:nif_not_loaded)
  def get_ack(_ref), do: :erlang.nif_error(:nif_not_loaded)
  def read(_ref, _num_bytes), do: :erlang.nif_error(:nif_not_loaded)
end
