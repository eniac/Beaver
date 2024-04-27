import paramiko # Need to be installed
import os
import time


def f_ssh_connection_create(ssh_ip, user_name, key_path):
  """Create a ssh connection to a specific node."""
  try:
    ssh_connection = paramiko.SSHClient()
    ssh_connection.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    private_key = paramiko.RSAKey.from_private_key_file(key_path)
    ssh_connection.connect(ssh_ip, username = user_name, pkey = private_key)
    return ssh_connection
  except paramiko.AuthenticationException:
    print("Authentication failed,"
          "please verify your credentials or ssh key path")
    return None
  except Exception as e:
    print(f"Could not establish SSH connection: {str(e)}")
    return None


def f_cmds_exec(ssh_connection, commands):
  """Execute multiple commands within the same SSH session."""
  for command in commands:
    channel = ssh_connection.get_transport().open_session()
    channel.exec_command(command)
    while not channel.exit_status_ready():
      time.sleep(1)
    channel.close()


def f_folder_remote_create(sftp, remote_folder_path):
  """Make new folder on remote node"""
  if remote_folder_path == '/':
    sftp.chdir('/')
    return
  if remote_folder_path == '':
    return
  try:
    sftp.chdir(remote_folder_path)
  except IOError:
    folder_name, folder_base = os.path.split(remote_folder_path.rstrip('/'))
    f_folder_remote_create(sftp, folder_name)
    sftp.mkdir(folder_base) 
    sftp.chdir(folder_base)
    return True


def f_folder_remote_send(ssh_connection, local_path, remote_path, folder_name):
  """Send local folder to remote directory."""
  sftp = ssh_connection.open_sftp()
  local_folder_path = os.path.join(local_path, folder_name)
  remote_folder_path = os.path.join(remote_path, folder_name)
  f_folder_remote_create(sftp, remote_folder_path)
  for root_path, dirs, files in os.walk(local_folder_path):
    for file in files:
      local_file_path = os.path.join(root_path, file)
      relative_path = os.path.relpath(local_file_path, local_folder_path)
      remote_file_path = os.path.join(remote_folder_path, relative_path)
      folder_path = os.path.dirname(remote_file_path)
      f_folder_remote_create(sftp, folder_path)
      sftp.put(local_file_path, remote_file_path)
  sftp.close()


def f_file_remote_send(ssh_connection, local_file_path, remote_file_path):
  """Send file under a specific folder to remote folder."""
  f_file_remote_delete(ssh_connection, remote_file_path)
  sftp = ssh_connection.open_sftp()
  sftp.put(local_file_path, remote_file_path)
  sftp.close()


def f_file_remote_fetch(ssh_connection, remote_file_path, local_file_path):
  """Fetch a file from a remote directory."""
  sftp = ssh_connection.open_sftp()
  local_path = os.path.dirname(local_file_path)
  os.makedirs(local_path, exist_ok=True)
  try:
    sftp.stat(remote_file_path)
    sftp.get(remote_file_path, local_file_path)
  except FileNotFoundError:
    print(f"Remote file not found: {remote_file_path}")
  finally:
    sftp.close()


def f_file_checker(ssh_connection, remote_file_path):
  sftp = ssh_connection.open_sftp()
  start_time = time.time()
  while (True):
    try:
      sftp.stat(remote_file_path)
      break  
    except IOError:
      elapsed_time = time.time() - start_time
      if elapsed_time > 60:
        print("Some programs may crash due to pressure test or the frequency "
              "is too low, please rerun the test.")
        exit(1)
      time.sleep(1) 
      

def f_folder_remote_delete(ssh_connection, remote_folder_path):
  """Delete remote folder."""
  commands = [f"rm -rf {remote_folder_path}"]
  f_cmds_exec(ssh_connection, commands)


def f_folder_files_remote_delete(ssh_connection, remote_folder_path):
  """Delete remote folder files."""
  commands = [f"rm -rf {remote_folder_path}/*"]
  f_cmds_exec(ssh_connection, commands)


def f_file_remote_delete(ssh_connection, remote_file_path):
  """Delete file in remote directory."""
  commands = [f"rm {remote_file_path}"]
  f_cmds_exec(ssh_connection, commands)


