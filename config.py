import csv
import struct

def encode_value(type, encoding, value):
    if encoding == 'string':
        return value.encode('utf-8') + b'\x00'  # 以null结尾的字符串
    elif encoding == 'u16':
        return struct.pack('<H', int(value))    # 16位无符号整数，little-endian
    else:
        raise ValueError(f'Unknown encoding: {encoding}')

def convert_csv_to_bin(csv_filename, bin_filename):
    with open(csv_filename, 'r') as csvfile, open(bin_filename, 'wb') as binfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            if row['type'] == 'namespace':
                continue  # 忽略 namespace 行
            value_bin = encode_value(row['type'], row['encoding'], row['value'])
            binfile.write(value_bin)

# 使用示例：
convert_csv_to_bin('config.cvs', 'config.bin')
