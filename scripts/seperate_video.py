import os,sys

def do_main(arg):
	vidname = arg
	basedir,basename = os.path.split(vidname)
	dirname = os.path.splitext(basename)[0]
	fout_pattern = "%s.%%05d.jpg"%dirname

	print "VIDEO IN   : ", arg
	print "DIR OUT    : ", dirname
	print "FILES OUT  : ", fout_pattern

	outdir = os.path.join(basedir,dirname)
	if not os.path.exists(outdir):
		os.mkdir(outdir)

	outpath = os.path.join(basedir,dirname,fout_pattern)

	cmd = "ffmpeg -i \"%s\" -s 853x480 -an \"%s\""%(vidname, outpath)
	print cmd
	os.system(cmd)

def main():
	for arg in sys.argv[1:]:
		do_main(arg)

if __name__ == '__main__':
	main()