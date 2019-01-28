http://www.yaffs.net/download-yaffs-using-git

We now operate a GIT server for our active projects. Here we give details of the setup:-

If you just want to browse or download code then it¡¦s easiest to access the repository over the web. 
This is also the best option if you are behind a firewall that does not allow direct GIT access (which is quite common).
To get a tarball of the top of the tree, just click ¡¥snapshot¡¦ for the release that you want, or to quickly obtain a snapshot of the top of the tree you can use this URL.
You will be offered a freshly-squeezed gzipped tarball of your choice.

If you want to use GIT directly from the command line then public read-only access is available, using the (bash) command:

git clone git://www.aleph1.co.uk/yaffs2 
If you are a registered developer with a server account you can get read/write access using:

git clone ssh://www.aleph1.co.uk/home/aleph1/git/yaffs2