Import("env")
import shutil, pathlib, gzip, os

def copy_data(src, dst):
  ext = pathlib.Path(src).suffix[1:]
  myfilename = "filelist.txt"
  drive, path_and_file = os.path.splitdrive(dst)
  path, file = os.path.split(path_and_file)
  print("dst: " + dst)
  print("drive: " + drive)
  print("path: " + path)
  print("file: " + file)
  myfile = open(drive + path + '\\' + myfilename, 'a')
  if (ext in ["js", "css", "html", "ico"]):
    myfile.write(file + ".gz\n")
    with open(src, 'rb') as src, gzip.open(dst + ".gz", 'wb') as dst:
      for chunk in iter(lambda: src.read(4096), b""):
        dst.write(chunk)
  else:
    myfile.write(file + '\n')
    shutil.copy(src, dst)
  myfile.close()

def copy_gzip_data(source, target, env):
  del_gzip_data(source, target, env)
  print("zipping html files")
  data = env.get("PROJECT_DATA_DIR")
  shutil.copytree(data[:-3], data, copy_function=copy_data)

def del_gzip_data(source, target, env):
  print("Clearing zipped files")
  data = env.get("PROJECT_DATA_DIR")
  shutil.rmtree(data, True)

env.AddPreAction("$BUILD_DIR/littlefs.bin", copy_gzip_data)
env.AddPostAction("$BUILD_DIR/littlefs.bin", del_gzip_data)