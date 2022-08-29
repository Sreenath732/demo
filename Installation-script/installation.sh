echo "This file copies installation folder to your machine in set_up folder."
echo "Do you wish to continue?"
select yn in "Yes" "No"; do
    case $yn in
        Yes )
	currpath=$(pwd)
	cd
	sudo mkdir /home/devel/set_up
	status=$?
	if [ $status -ne 0 ];then
		echo "Failed to create set_up directory"
		exit
	fi
	sudo chmod 777 /home/devel/set_up
	scp $currpath/configuration_script/* /home/devel/set_up
	status=$?
	if [ $status -ne 0 ];then
		echo "Failed to copy files to set_up directory"
		exit
	fi
	echo "Files copied successfully."
	sudo chmod 777 /home/devel/set_up/*
	cd /home/devel/set_up
	./configuration.sh
	echo "Installation done Successfully"
	break;;
        No ) exit;;
    esac
done
