# 556_proj2

Group Member: Baodong Chen, Zewen Rao, Xiaoxi Xu, Ziwei Zhang

We use stop-and-wait to implement the transmission from the sender to the receiver. The data format we are using is listed as below:
- DGRAM Packet Structure
- Packet Type: SEQ or FIN: 1 Byte (0:SEQ, 1: FIN)
- sq_no: sequence number: 1 byte
- Packet Size: 4 Bytes 
- Directory: 60 Bytes
- File name: 20 Bytes
- Data: 1382 Bytes
- CRC Error Code: 4 Bytes

We use CRC to check for errors in the data, and checksum to check for errors in the acknowledgement. 
