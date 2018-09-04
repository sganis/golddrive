# unmount drive

if __name__ == '__main__':

	import sys
	assert len(sys.argv) > 1 # usage: unmount.py <drive>

	logging.basicConfig(level=logging.INFO)
	import mount
	mount.unmount(sys.argv[1])