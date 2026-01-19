!#/bin/bash
targetPath="LabelMaster/opt/labelmaster/labelmaster"
binPath="bin/LabelMaster"
if [ -f ${binPath} ]
then
	rm ${binPath}
fi
cmake --build build
if [ -f ${targetPath} ]
then 
	rm ${targetPath}
fi	
mv ${binPath} ${targetPath}
dpkg-deb --build LabelMaster labelmaster_1.2.2_amd64.deb
mv labelmaster_1.2.2_amd64.deb ../dataset
