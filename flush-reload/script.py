import sys
import string

i=0

for line in sys.stdin:
	i=i+1
	if i >= 3:
		ls = line.split('|')
		x=False
		y=False
		prev=False
		prev1=True
		for elem in ls:
			for j in range(len(elem)):
				if elem[j]=='A':
					x=True
				if elem[j]=='B':
					y=True
			if x and y and prev and prev1:
				print("S|",end='')
			elif y:
				print("U|",end='')
				prev= True
			elif x:
				print("S|",end='')
				prev=False
				prev1=False
		print("\n")
