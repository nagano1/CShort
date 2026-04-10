containerWorkspaceFolder=$1
build_persistent=$2
echo $containerWorkspaceFolder
echo $build_persistent
     
sudo rm -rf /tmp/build
mkdir -p $containerWorkspaceFolder/$build_persistent
sudo chown vscode:vscode $containerWorkspaceFolder/$build_persistent
ln -s $containerWorkspaceFolder/$build_persistent /tmp/build