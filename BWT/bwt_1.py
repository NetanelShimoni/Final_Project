def BWT(sequence):
	sequence += '$'
	table = [sequence[index:] + sequence[:index] for index, _ in enumerate(sequence)]
	table.sort()
	bwt = [rotation[-1] for rotation in table]
	bwt = ''.join(bwt)

	return bwt


def inverse_BWT(sequence):
	table = [col for col in sequence]
	for i in range(len(sequence) - 1):
		table.sort()
		table = [sequence[i] + table[i] for i in range(len(sequence))]

	return table[[row[-1] for row in table].index('$')]


#main!!!
name = 'alice29'
with open(name + '.txt', 'r') as file:
    data = file.read().replace('\n', '\n')
bwt = BWT(data)
f = open(name + '-c.txt', "w")
f.write(str(bwt))
f.close()
#print('Burrows-Wheeler Transform: ' + str(bwt))

inverse = inverse_BWT(bwt)
print('Inverse Burrows-Wheeler Transform: ' + str(inverse))