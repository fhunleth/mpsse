defmodule MPSSE do
  @moduledoc """
  Documentation for `MPSSE`.
  """

  @type mode() :: :spi0 | :spi1 | :spi2 | :spi3 | :i2c | :gpio | :bit_bang
  @type speed() ::
          100_000
          | 400_000
          | 1_000_000
          | 2_000_000
          | 5_000_000
          | 6_000_000
          | 10_000_000
          | 12_000_000
          | 15_000_000
          | 30_000_000
          | 60_000_000

  @type endian() :: :msb | :lsb

  alias MPSSE.NIF

  @spec find_and_open(mode(), speed(), endian()) :: {:ok, reference()}
  def find_and_open(mode, speed \\ nil, endian \\ nil) do
    NIF.open(mode(mode), speed(mode, speed), endian(mode, endian))
  end

  defp mode(:spi0), do: 1
  defp mode(:spi1), do: 2
  defp mode(:spi2), do: 3
  defp mode(:spi3), do: 4
  defp mode(:i2c), do: 5
  defp mode(:gpio), do: 6
  defp mode(:bit_bang), do: 7

  defp speed(:i2c, nil), do: 400_000
  defp speed(_, x) when x <= 100_000, do: 100_000
  defp speed(_, x) when x <= 400_000, do: 400_000
  defp speed(_, x) when x <= 1_000_000, do: 1_000_000
  defp speed(_, x) when x <= 2_000_000, do: 2_000_000
  defp speed(_, x) when x <= 5_000_000, do: 5_000_000
  defp speed(_, x) when x <= 6_000_000, do: 6_000_000
  defp speed(_, x) when x <= 10_000_000, do: 10_000_000
  defp speed(_, x) when x <= 12_000_000, do: 12_000_000
  defp speed(_, x) when x <= 15_000_000, do: 15_000_000
  defp speed(_, x) when x <= 30_000_000, do: 30_000_000
  defp speed(_, _x), do: 60_000_000

  defp endian(:i2c, nil), do: 0x0
  defp endian(_mode, :msb), do: 0x0
  defp endian(_mode, :lsb), do: 0x8

  @spec close(reference()) :: :ok
  defdelegate close(mpsse), to: NIF

  @spec start(reference()) :: :ok
  defdelegate start(mpsse), to: NIF

  @spec stop(reference()) :: :ok
  defdelegate stop(mpsse), to: NIF

  @spec write(reference(), iodata()) :: :ok
  defdelegate write(mpsse, data), to: NIF

  @spec get_ack(reference()) :: :ack | :nak
  def get_ack(mpsse) do
    case NIF.get_ack(mpsse) do
      0 -> :ack
      _ -> :nak
    end
  end

  @spec read(reference(), non_neg_integer()) :: {:ok, binary()}
  defdelegate read(mpsse, length), to: NIF
end
