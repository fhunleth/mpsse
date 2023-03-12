defmodule EEPROM do
  def doit() do
    with {:ok, mpsse} <- MPSSE.find_and_open(:i2c),
         :ok <- MPSSE.start(mpsse),
         :ok <- MPSSE.write(mpsse, <<0xA0, 0, 0>>),
         :ack <- MPSSE.get_ack(mpsse),
         :ok <- MPSSE.start(mpsse),
         :ok <- MPSSE.write(mpsse, <<0xA1>>),
         :ack <- MPSSE.get_ack(mpsse),
         {:ok, result} <- MPSSE.read(mpsse, 16) do
      MPSSE.stop(mpsse)
      result
    end
  end
end
