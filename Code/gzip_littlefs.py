Import("env")
import shutil, pathlib, gzip, os

from webInterface.build import build_web_interface

def copy_data(src, dst):

  print(f" copy_data: {src} {dst}")

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
  data_zip_dir = env.get("PROJECT_DATA_DIR")
  data_dir = data_zip_dir[:-3]
  web_interface_base_dir = os.path.join(data_dir[:-4], "webInterface")

  print(f"creating '{data_dir}' if not exists")
  if not os.path.exists(data_dir):
    os.makedirs(data_dir)

  print(f"Building web interface from {web_interface_base_dir} into {data_dir}")
  build_web_interface(data_dir, web_interface_base_dir)

  print("Copy base data files")
  data_base_dir = data_dir + '_base'
  if os.path.exists(data_base_dir):
    for item in os.listdir(data_base_dir):
      src = os.path.join(data_base_dir, item)
      dst = os.path.join(data_dir, item)
      if os.path.isdir(src):
        shutil.copytree(src, dst, dirs_exist_ok=True)
      else:
        shutil.copy2(src, dst)

  print("zipping html files")
  shutil.copytree(data_dir, data_zip_dir, copy_function=copy_data)

def del_gzip_data(source, target, env):
  print("Clearing zipped files")
  data = env.get("PROJECT_DATA_DIR")
  shutil.rmtree(data, True)

env.AddPreAction("$BUILD_DIR/littlefs.bin", copy_gzip_data)
env.AddPostAction("$BUILD_DIR/littlefs.bin", del_gzip_data)