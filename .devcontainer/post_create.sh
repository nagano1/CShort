containerWorkspaceFolder=$1
build_persistent=$2
echo $containerWorkspaceFolder
echo $build_persistent
     
sudo rm -rf /tmp/build
# Create the persistent build directory and set permissions.
mkdir -p $containerWorkspaceFolder/$build_persistent
sudo chown vscode:vscode $containerWorkspaceFolder/$build_persistent
# Create a symbolic link to the persistent build directory. 
# This allows build tools to use /tmp/build as the build directory, while the actual files are stored in the persistent location. 
ln -s $containerWorkspaceFolder/$build_persistent /tmp/build