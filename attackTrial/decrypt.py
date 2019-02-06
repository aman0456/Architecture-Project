from __future__ import print_function
import matplotlib.pyplot as plt
f = open("timing.txt","r")
a = f.read()

temp = a.split("\n")

temp = temp[:-10]
# arr = [float(i) for i in temp]
arr = [float(i) for i in temp]

initial_seq = [1]*5 + [0]*5 + [1]
# print( initial_seq)
length = 10

cutoff = 233

for i in range(0,len(arr) - len(initial_seq)*length ):
	valid = True
	for j in range(0,len(initial_seq)):
		summ = 0
		for k in range(i+j*length,i+(j+1)*length):
			summ += arr[k]
		avg = summ/length
		if not((avg < cutoff and initial_seq[j] == 0) or (avg >= cutoff and initial_seq[j] == 1)):
			valid = False
	if valid:
		break

count_arr = [0,0]
sum_arr = [0,0]
for j in range(0,len(initial_seq)):
	count_arr[initial_seq[j]] += 1
	sum_arr[initial_seq[j]] += sum(arr[i+j*length: i+(j+1)*length])/length

avg_arr = [0,0]
avg_arr[0] = sum_arr[0]/count_arr[0]
avg_arr[1] = sum_arr[1]/count_arr[1]
# print(count_arr[0], sum_arr[0], avg_arr[0])
# print(count_arr[1], sum_arr[1], avg_arr[1])
cutoff = (avg_arr[0] + avg_arr[1])/2

# print("cutoff =",cutoff)
originalstart = i + length*len(initial_seq)
for cutOffRange in range(1):
	cutoff += 1
	number = 0
	power = 1
	received = []
	start = originalstart
	while True:
		for i in range(start,start+8*length,length):
			avg = sum(arr[i:i+length])
			avg = avg/length
			if avg > cutoff:
				number += power
			power *= 2
		if number == 0:
			break
		else:
			received.append(chr(number))
			number = 0
			power = 1
		start += 8*length
	received_string = "".join(received)
	print("Received string = " + received_string)

plt.plot(arr)
plt.xlabel('Memory Accesses')
plt.ylabel("Time for the Access")
plt.show()
