#dimmer value
def dimmer(comb, stddev_c):
    li = comb / stddev_c
    if li>1: li=1
    return li
