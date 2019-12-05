#!/bin/bash
#git clone https://wangfei32@icode.baidu.com:8235/baidu/codescan/bcacpp
#cd bcacpp
#git branch
rm -rf bcacpp_temp
mv bcacpp bcacpp_temp
echo -n "please enter the version:"
read var
APP_PATH=bcacpp$var
rm -rf $APP_PATH
mkdir $APP_PATH
cd bcacpp_temp/
chmod u+x install.sh
make clean && make
cd ../
cp -r bcacpp_temp/addons/ bcacpp_temp/check_list.xml bcacpp_temp/htmlreport/ bcacpp_temp/platforms/ bcacpp_temp/rules/ bcacpp_temp/cfg/ bcacpp_temp/cppcheck bcacpp_temp/externals/ bcacpp_temp/install.sh $APP_PATH/
tar -czvf $APP_PATH.tar.gz $APP_PATH/


