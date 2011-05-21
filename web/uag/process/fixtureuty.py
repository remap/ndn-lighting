import namespaces
def alloff(delay,out,fade):
    for n in namespaces.name:
        out.append((delay,n,(0,0,0),0,fade))
    return out
