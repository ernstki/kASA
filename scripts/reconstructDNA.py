import sys, math

code = {
"TTT": "F","TTC": "F", "TTA":"L", "TTG":"L", 
"CTT":"L", "CTC":"L", "CTA":"L", "CTG":"L", 
"ATT":"I", "ATC":"I", "ATA":"I", "ATG":"M", 
"GTT":"V", "GTC":"V", "GTA":"V", "GTG":"V", 

"TCT":"S", "TCC":"S", "TCA":"S", "TCG": "S",
"CCT":"P", "CCC":"P", "CCA":"P", "CCG":"P",
"ACT":"T", "ACC":"T", "ACA":"T", "ACG":"T",
"GCT":"A", "GCC":"A", "GCA":"A", "GCG":"A",

"TAT":"Y", "TAC":"Y", "TAA":"[", "TAG":"[",
"CAT":"H", "CAC":"H", "CAA":"Q", "CAG":"Q",
"AAT":"N", "AAC":"N", "AAA":"K", "AAG":"K",
"GAT":"D", "GAC":"D", "GAA":"E", "GAG":"E",

"TGT":"C", "TGC":"C", "TGA":"]", "TGG":"W",
"CGT":"R", "CGC":"R", "CGA":"R", "CGG":"R",
"AGT":"B", "AGC":"B", "AGA":"R", "AGG":"R",
"GGT":"G", "GGC":"G", "GGA":"G", "GGG":"G"
}

code_16 = {
"TTT":"A","TTC": "A", "TTA":"A", "TTG":"A", 
"CTT":"B", "CTC":"B", "CTA":"B", "CTG":"B", 
"ATT":"C", "ATC":"C", "ATA":"C", "ATG":"C", 
"GTT":"D", "GTC":"D", "GTA":"D", "GTG":"D", 

"TCT":"E", "TCC":"E", "TCA":"E", "TCG":"E",
"CCT":"F", "CCC":"F", "CCA":"F", "CCG":"F",
"ACT":"G", "ACC":"G", "ACA":"G", "ACG":"G",
"GCT":"H", "GCC":"H", "GCA":"H", "GCG":"H",

"TAT":"I", "TAC":"I", "TAA":"I", "TAG":"I",
"CAT":"J", "CAC":"J", "CAA":"J", "CAG":"J",
"AAT":"K", "AAC":"K", "AAA":"K", "AAG":"K",
"GAT":"L", "GAC":"L", "GAA":"L", "GAG":"L",

"TGT":"M", "TGC":"M", "TGA":"M", "TGG":"M",
"CGT":"N", "CGC":"N", "CGA":"N", "CGG":"N",
"AGT":"O", "AGC":"O", "AGA":"O", "AGG":"O",
"GGT":"P", "GGC":"P", "GGA":"P", "GGG":"P"
}

inputSequence = sys.argv[1]

switch = 0
aaSequences = ["","",""]

for i in range(len(inputSequence)):
	if i + 2 < len(inputSequence):
		aaSequences[switch] += code[inputSequence[i:i+3]]
		switch = (switch + 1) % 3

print("Frame 1:",aaSequences[0], "Frame 2:",aaSequences[1], "Frame 3:", aaSequences[2])

codeTTPOMinus1 = {
"F" : ["TTT","TTC"], 
"L" : ["TTA","TTG","CTT","CTC","CTA","CTG"], 
"I" : ["ATT","ATC","ATA"], 
"M":["ATG"], 
"V" : ["GTT","GTC","GTA","GTG"], 
"P" : ["CCT", "CCC", "CCA", "CCG"], 
"T" : ["ACT", "ACC", "ACA", "ACG"],
"A" : ["GCT","GCC", "GCA", "GCG"], 
"Y" : ["TAT", "TAC"], 
"[" : ["TAA", "TAG"], 
"H" : ["CAT","CAC"], 
"Q" : ["CAA", "CAG"], 
"N" : ["AAT", "AAC"], 
"K" : ["AAA", "AAG"], 
"D" : ["GAT", "GAC"], 
"E": ["GAA", "GAG"], 
"C": ["TGT", "TGC"], 
"]" : ["TGA"],
"W" : ["TGG"], 
"R" : ["CGT", "CGC", "CGA", "CGG", "AGA", "AGG"], 
"S" : ["TCT", "TCC", "TCA", "TCG"], 
"B" : ["AGT", "AGC"], 
"G" : ["GGT", "GGC", "GGA", "GGG"]
}

code_16TTPOMinus1 = {
"A" : ["TTT","TTC","TTA","TTG"], 
"B" : ["CTT","CTC","CTA","CTG"], 
"C" : ["ATT","ATC","ATA","ATG"], 
"D" : ["GTT","GTC","GTA","GTG"], 
"E" : ["TCT", "TCC", "TCA", "TCG"], 
"F" : ["CCT", "CCC", "CCA", "CCG"], 
"G" : ["ACT", "ACC", "ACA", "ACG"],
"H" : ["GCT","GCC", "GCA", "GCG"], 
"I" : ["TAT", "TAC","TAA", "TAG"], 
"J" : ["CAT","CAC","CAA", "CAG"], 
"K" : ["AAT", "AAC","AAA", "AAG"], 
"L" : ["GAT", "GAC","GAA", "GAG"], 
"M" : ["TGT", "TGC","TGA","TGG"], 
"N" : ["CGT", "CGC", "CGA", "CGG"], 
"O" : ["AGA", "AGG","AGT", "AGC"], 
"P": ["GGT", "GGC", "GGA", "GGG"]
}

reconstructedSequence = ""
resultArr = []

for i in range(len(aaSequences[0])):
	#print("Iteration: ",i)
	triplets0 = codeTTPOMinus1[aaSequences[0][i]]
	triplets1 = codeTTPOMinus1[aaSequences[1][i]] if  i < len(aaSequences[1]) else []
	triplets2 = codeTTPOMinus1[aaSequences[2][i]] if  i < len(aaSequences[2]) else []
	
	#print(triplets0, triplets1, triplets2, resultArr)
	
	found  = False
	for elem in resultArr:
		#print(reconstructedSequence, elem, triplets0[0])
		if elem[3] == triplets0[0][0] and elem[4] == triplets0[0][1]:
			if reconstructedSequence == "":
				reconstructedSequence += elem
			else:
				#print(elem)
				reconstructedSequence += elem[2] + elem[3] + elem[4]
			found = True
			break
	if not found and reconstructedSequence != "":
		print("error, wrong order!")
		break
	
	resultArr = []
	
	for entry0 in triplets0:
		if triplets1 != []:
			for entry1 in triplets1:
				if triplets2 != []:
					for entry2 in triplets2:
						if entry0[1] == entry1[0] and entry0[2] == entry2[0] and entry1[1] == entry2[0] and entry1[2] == entry2[1]:
							resultArr.append(entry0 + entry1[2] + entry2[2])
				else:
					if entry0[1] == entry1[0] and entry0[2] == entry1[1]:
						resultArr.append(entry0 + entry1[2])
		else:
			resultArr.append(entry0)

#print(resultArr)

found  = False
for elem in resultArr:
	#print(reconstructedSequence, elem)
	if reconstructedSequence != "":
		if len(elem) == 5:
			if elem[0] == reconstructedSequence[-2] and elem[1] == reconstructedSequence[-1]:
				reconstructedSequence += elem[2] + elem[3] + elem[4]
				found = True
				break
		elif len(elem) == 4:
			if elem[0] == reconstructedSequence[-2] and elem[1] == reconstructedSequence[-1]:
				reconstructedSequence += elem[2] + elem[3]
				found = True
				break
		else:
			if elem[0] == reconstructedSequence[-2] and elem[1] == reconstructedSequence[-1]:
				reconstructedSequence += elem[2]
				found = True
				break
	else:
		reconstructedSequence += elem
		found = True
		break

if not found or len(reconstructedSequence) != len(inputSequence): #the latter could be replaced by 3*len(aaSequences[2]) + 2 assuming aaSequences[2] is the shortest amino acid-like sequence
	print("error, wrong order!")
	sys.exit()

print(inputSequence)
matches = ""
for i in range(len(inputSequence)):
	if inputSequence[i] == reconstructedSequence[i]:
		matches += "|"
	else:
		matches += " "

print(matches)
print(reconstructedSequence)