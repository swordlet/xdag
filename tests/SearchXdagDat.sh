#!/bin/bash
# get the number of functions in RTS code

# $1是运行脚本时，输入的第一个参数，这里指的是使用者希望搜索的目录
# 下面的代码是对目录进行判断，如果为空则使用脚本所在的目录；否则，搜索用户输入的目录
if [[ -z "$1" ]] || [[ ! -d "$1" ]]; then
    echo "The directory is empty or not exist!"
    echo "It will use the current directory."
    nowdir=$(pwd)
else
    nowdir=$(cd $1; pwd)
fi
echo "$nowdir"

# 递归函数的实现
function SearchCfile()
{
    cfilelist=$(ls | grep '\.dat')
    for cfilename in $cfilelist
     do
	filesize=$(ls -l $cfilename | awk '{ print $5 }')
	if [ $filesize -gt 1048576 ];then
	 	echo "$cfilename $filesize"
	fi
     done
    
    # 遍历当前目录，当判断其为目录时，则进入该目录递归调用该函数；
    dirlist=$(ls)
    #echo "dirlist is $dirlist" 
    for dirname in $dirlist
     do
        if [ -d "$dirname" ];then
            cd $dirname
            SearchCfile
            cd ..
        fi
     done
}

# 调用上述递归调用函数
SearchCfile
