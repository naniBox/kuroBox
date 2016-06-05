import os,sys,glob

def do_main(arg):
	vidname = arg
	basedir,basename = os.path.split(vidname)
	dirname = os.path.splitext(basename)[0]
	fout_pattern = "%s.%%05d.jpg"%dirname

	print "VIDEO IN   : ", arg
	print "DIR OUT    : ", dirname
	print "FILES OUT  : ", fout_pattern
	print "-"*80
	print ""

	outdir = os.path.join(basedir,dirname)
	if not os.path.exists(outdir):
		os.mkdir(outdir)

	outpath = os.path.join(basedir,dirname,fout_pattern)

	vres = 720
	hres = int(16.0/9.0*vres)

	cmd = "ffmpeg -i \"%s\" -s %dx%d -an \"%s\""%(vidname, hres, vres, outpath)
	print cmd
	os.system(cmd)
	os.rename(vidname, os.path.join(dirname, vidname))

def main():
	for arg in sys.argv[1:]:
		for g in glob.glob(arg):
			do_main(g)

if __name__ == '__main__':
	main()