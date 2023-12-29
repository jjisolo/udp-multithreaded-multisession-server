import struct
from typing import *

THREAD_CACHE_MAGIC = 0x595A5A4F
THREAD_CACHE_START = 0xDEADBEEF
THREAD_CACHE_END   = 0xC0FFEE

def read_result_file() -> List[List[str]]:
    clients_data       : List[List[str]] = []
    current_client_data: List[str]       = []

    with open("result.bin", "rb") as file:
        print("Parsing result.bin file")

        header_data = file.read(4)
        if(not header_data):
            return
        header = int.from_bytes(header_data, byteorder='little')

        if header != THREAD_CACHE_MAGIC:
            print("Invalid file to process!")
            raise NotImplementedError()

        file.read(4) # Skip THREAD_START directive

        while True:
            hex_data = file.read(8)
            if(not hex_data):
                break

            int_value = int.from_bytes(hex_data, byteorder='little')
            if hex(int_value & 0xFFFFFF) == hex(THREAD_CACHE_END):
                clients_data.append(current_client_data.copy())
                current_client_data.clear()
                continue

            double_value = struct.unpack('d', int_value.to_bytes(8, byteorder='little'))[0]
            current_client_data.append(double_value)

        return clients_data

def print_result_file(result: List[List[str]]) -> None:
    total    = 0

    for result_elem in result:
        print("\n\n")
        print("="*8 + " CACHED ENTRY " + "="*8)
        total    = len(result_elem)
        counter  = 1
        to_print = []
        
        for list_elem in result_elem:
            if counter % 5 == 0:
                to_print.append(list_elem)
                print("\t".join(map(str, to_print)))
                to_print.clear()
                counter += 1
                continue

            to_print.append(list_elem)
            counter += 1
        print("\t".join(map(str, to_print)))

        print("-----")
        print(f"Elements total: {total}")
        print("\n\n")

if __name__ == "__main__":
    print_result_file(read_result_file())
