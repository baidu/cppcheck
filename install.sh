echo "install start ..."
BIN=/usr/bin
install -d $BIN
echo "install bcacpp ..."
install cppcheck $BIN
echo "install bca_check_list.xml ..."
install check_list.xml $BIN
install addons/*.py $BIN
install addons/*/*.py $BIN
install htmlreport/cppcheck-htmlreport $BIN
echo "install cfg ..."
install -m 644 cfg/* $BIN
echo "install finished ..."

