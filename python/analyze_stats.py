import pstats, sys

p = pstats.Stats(sys.argv[1])

print "top 10 cumulative:"
# cumulative function time, top 10
p.strip_dirs().sort_stats('cumulative').print_stats(15)

print "top 10 loops:"
# most time in loops
p.strip_dirs().sort_stats('time').print_stats(15)

print "overall:"
p.strip_dirs().sort_stats(-1).print_stats()
print "overall by name:"
p.sort_stats('name')
p.print_stats()