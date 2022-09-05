
# run "protoc --python_out=. sample.proto" to make this
import SampleProto_pb2
from google.protobuf import message

def main_test1():
    m = SampleProto_pb2.Client2Server()
    m.type = SampleProto_pb2.MessageType.LOGIN
    m.login.user = "pfk"
    m.login.password = "pass"
    body = m.SerializeToString()
    length = len(body)   # m.ByteSize()
    encoded = length.to_bytes(2, byteorder='big') + body
    # note 'encoded' is 'bytes', not 'str'
    # so no need to .encode() or use ord(c).
    print(" ".join("{:02x}".format(c) for c in encoded))

    length = int.from_bytes(encoded[0:2], byteorder='big')
    print(f'decoded length {length}')
    n = SampleProto_pb2.Client2Server()
    try:
        n.ParseFromString(encoded[2:2+length])
        print('decoded:', n)
    except message.DecodeError as e:
        print('exception type', type(e), ':\n', e)
