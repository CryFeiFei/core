import os
import hashlib
import zipfile
import tarfile

def uncompress_file_to_dir(compressed_file, uncompress_dir):
    command = None
    extension = os.path.splitext(compressed_file)[1]

    try:
        os.mkdir(uncompress_dir)
    except FileExistsError as e:
        pass

    if extension == '.gz':
        tar = tarfile.open(compressed_file)
        tar.extractall(uncompress_dir)
        tar.close()
    elif extension == '.zip':
        zip_file = zipfile.ZipFile(compressed_file)
        zip_file.extractall(uncompress_dir)
        zip_file.close()

        uncompress_dir = os.path.join(uncompress_dir, os.listdir(uncompress_dir)[0])
        if " " in os.listdir(uncompress_dir)[0]:
            print("replacing whitespace in directory name")
            os.rename(os.path.join(uncompress_dir, os.listdir(uncompress_dir)[0]),
                            os.path.join(uncompress_dir, os.listdir(uncompress_dir)[0].replace(" ", "_")))
    else:
        print("Error: unknown extension " + extension)

    return os.path.join(uncompress_dir, os.listdir(uncompress_dir)[0])

BUF_SIZE = 1048576

def get_hash(file_path):
    sha512 = hashlib.sha512()
    with open(file_path, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            sha512.update(data)
        return sha512.hexdigest()

def get_file_info(mar_file, url):
    filesize = os.path.getsize(mar_file)
    data = { 'hash' : get_hash(mar_file),
            'hashFunction' : 'sha512',
            'size' : filesize,
            'url' : url + os.path.basename(mar_file)}

    return data

def replace_variables_in_string(string, **kwargs):
    new_string = string
    for key, val in kwargs.items():
        new_string = new_string.replace('$(%s)'%key, val)

    return new_string

def make_complete_mar_name(target_dir, filename_prefix):
    filename = filename_prefix + "_complete.mar"
    return os.path.join(target_dir, filename)
