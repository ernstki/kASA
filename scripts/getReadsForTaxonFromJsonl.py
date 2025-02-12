import sys, getopt, os, json

def getReads(argv):
	threshold = 0.0
	
	try:
		opts, args = getopt.getopt(argv, "i:f:o:t:", [])
	except getopt.GetoptError:
		print('-i <json input> -f <fastq or fasta> -o <output> -t <tax id>')
	
	for opt, arg in opts:
		if opt in ("-i",):
			kASAOutput = open(arg)
		if opt in ("-f",):
			fasta_q = open(arg)
		elif opt in ("-o",):
			outFile = open(arg, 'w')
		elif opt in ("-t",):
			taxID = arg
	
	resultSet = set()
	
	for read in kASAOutput:
		read = json.loads(read)
		taxa = read["Top hits"]
		
		if len(taxa) != 0:
			if (taxa[0]["tax ID"] == taxID):
				resultSet.add(read["Specifier from input file"])
	
	
	fastqora = True
	firstLine = next(fasta_q)
	if firstLine[0] == "@":
		fastqora = True
	else:
		fastqora = False
	fasta_q.seek(0)
	
	
	writeStuff = False
	for line in fasta_q:
		if "@" in line or ">" in line:
			t_line = (line.rstrip("\r\n")).lstrip("@>")
			if t_line in resultSet:
				if fasta_q:
					outFile.write(line + next(fasta_q) + next(fasta_q) + next(fasta_q))
				else:
					outFile.write(line)
				writeStuff = True
			else:
				writeStuff = False
		else:
			if writeStuff:
				outFile.write(line)

getReads(sys.argv[1:])