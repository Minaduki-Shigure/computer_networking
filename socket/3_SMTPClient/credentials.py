import base64
name = b'i@minaduki.cn'
passwd = b'YOUR_PASSWORD'

def base64_output():
    username = base64.encodestring(name).decode('ascii')
    password = base64.encodestring(passwd).decode('ascii')
    return username, password