import random

# random strings happening here

random.seed(a=None, version=2)
line = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
for y in range (0,3):
    my_string=""

    for x in range (0,10):
       line[x]= random.randint(97,122)

    for x in range(0,10):
       my_string+=chr(line[x])

    print (my_string)
    if y == 0:
       file_one = open("file_one", "w+")
       file_one.write("%s\n" % my_string)
       file_one.close()
    elif y==1:
       file_two = open("file_two", "w+")
       file_two.write("%s\n" % my_string)
       file_two.close()
    else:
       file_three = open("file_three", "w+")
       file_three.write("%s\n" % my_string)
       file_three.close()

#random numbers happening here

num_a = random.randint(1,42)
num_b = random.randint(1,42)

num_product = num_a * num_b

print (num_a)
print (num_b)
print (num_product)

