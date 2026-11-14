def test(msgtype, num):
    if num <= 1:
        msgtype = 1
    elif 2 < num <= 3:
        msgtype = 3
    else:
        msgtype = 4
    return msgtype
