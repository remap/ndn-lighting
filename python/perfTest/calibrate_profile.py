import profile

# calibrate per-system... increase c until you get predictable results.
c = 10000
pr = profile.Profile()
for i in range(5):
    print pr.calibrate(c)



# 1. Apply computed bias to all Profile instances created hereafter.
#profile.Profile.bias = your_computed_bias

# 2. Apply computed bias to a specific Profile instance.
#pr = profile.Profile()
#pr.bias = your_computed_bias

# 3. Specify computed bias in instance constructor.
#pr = profile.Profile(bias=your_computed_bias)