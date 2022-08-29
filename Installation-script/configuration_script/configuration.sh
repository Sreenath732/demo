echo "The installation process need the super user password."
echo "Do you wish to continue?"
select yn in "Yes" "No"; do
    case $yn in
        Yes )
	df -h | awk '{ print $6 "-" $4 }' > dirfile.txt
	sed -i '1d' dirfile.txt
	echo "Choose the data directory for postgresql from the below list."
	dirfile="dirfile.txt"
	dirs=`cat $dirfile`
	currpath=$(pwd)
	select dir in $dirs
	do
	echo "You have chosen $dir"
	dir=${dir%-*}
	if [ "$dir" != "" ]; then
	# Checking postgresql
	if [ -d /etc/postgresql ]; then
	   	echo "Postgresql was already installed in your system, do you want to skip reinstallation of postgresql? Or Do you want to reinstall postgresql(if you go with this you will loose all previous data in database).?"
	   	select yn in "Skip" "Reinstall" "Exit"; do
			case $yn in
		        Skip )
			echo "Step 1/11 Reinstallation of postgresql Skipped"   
		    sudo service plcdatacollector stop
		    sudo service alarmdatacollector stop
		    sudo service dataloggerapi stop
		    sudo service kpiloggerapi stop
		    sudo service commissioning-server stop
		    sudo service postgresql restart	
		    status=$?
		    if [ $status -ne 0 ]; then
		    	echo "service not started"
		    fi	
			val=$(df -h | awk ''$dir'/ { print $5 }')
			val=${val::-1}
			#echo "val="$val
			if [ $val -le 50 ] ; then

				echo "Step 2/11 Checking new database as plc_data"
				status=$(sudo -u postgres psql -qAt -c "select exists( SELECT datname FROM pg_catalog.pg_database WHERE lower(datname) = lower('plc_data'));")
				#echo $status
				if [ $status == 't' ]; then
					echo "Database plc_data already exist"
					#exit
				else
					sudo -u postgres psql -c 'CREATE DATABASE plc_data;'
					status=$?
					if [ $status -ne 0 ]; then
				   		echo "Installation failed at step2"
				   		exit
					fi	
				fi	

				echo "Step 3/11 Checking timescaledb extension on plc_data"
				status=$(sudo -u postgres psql -qAt -d 'plc_data' -c "select exists (select FROM pg_available_extensions where name = 'timescaledb');")
				if [ $status == 't' ]; then
					echo "timescaledb already added on plc_data"
				else
					sudo -u postgres psql -d 'plc_data' -c 'CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;'
					status=$?
					if [ $status -ne 0 ]; then
						echo "Installation failed at step3"
						exit
					fi	
				fi
				echo "Step 4/11 Checking new table as data_table"
				status=$(sudo -u postgres psql -qAt -d 'plc_data' -c "SELECT EXISTS (SELECT table_name FROM information_schema.tables WHERE table_name = 'data_table');")
				#echo $status
				if [ $status == 't' ]; then
					echo "Table data_table already exist"
					#exit
				else
					./dbdc -f CART.AWL -P 5432
					status=$?
					if [ $status -ne 0 ];then
				   		echo "Unable to create tables in database"
				   		exit
					fi
					echo "Create a view for data_table"
					sudo -u postgres psql -d 'plc_data' -c "create or replace view data_view as select * from data_table;"
					sudo -u postgres psql -d 'plc_data' -c "CREATE TABLE alarm_defs(paramnameFirst VARCHAR(100),alarm_id INT DEFAULT 0,paramnameSecond VARCHAR(100),db_field_name VARCHAR(20),bit_number INT DEFAULT 0,field_type VARCHAR(5));"
				fi

			else
				echo "Not Enough space for further Installation.Kindly take a backup of existing"
				echo "Use the backup_restore.sh file to backup"

			fi

			break;;
		        Reinstall )
			directory=$( sudo -u postgres psql -t -c "SHOW data_directory" )
		    sudo service postgresql stop     
		    sudo service plcdatacollector stop
		    sudo service alarmdatacollector stop
		    sudo service dataloggerapi stop
		    sudo service kpiloggerapi stop
		    sudo service commissioning-server stop
			sudo service postgresql stop
			sudo apt --fix-broken install
			status=$?
			if [ $status -ne 0 ];then
				echo "Postgresql service not stopped"
				#exit
			fi   
			sudo apt-get --purge remove postgresql-11 postgresql-client-11 postgresql-client-common postgresql-common
			status=$?
			if [ $status -ne 0 ];then
				echo "Postgresql not removed"
				exit
			fi
			sudo rm -d /etc/postgresql
			status=$?
			if [ $status -ne 0 ];then
				echo "/etc/postgresql not removed"
				#exit
			fi
			sudo rm -d /etc/postgresql-common
			status=$?
			if [ $status -ne 0 ];then
				echo "/etc/postgresql-common not removed"
				#exit
			fi
			sudo rm -rf $directory
			status=$?
			if [ $status -ne 0 ];then
				echo "postgres data directory not removed"
				#exit
			fi
			echo "Postgresql removed"
			echo "Step 1/11 Installation of postgresql-11 and timescaledb."
			sudo dpkg -i *.deb
			status=$?
			if [ $status -ne 0 ];then
				echo "Installation failed at step1"
				exit
			fi
			pgVesrion=$( sudo -u postgres psql -c "SHOW server_version;" )
			pgVesrion=`echo $pgVesrion | tr -dc '[:alnum:]\n\r' | tr '[:upper:]' '[:lower:]'`
			pgVesrion=`echo $pgVesrion | tr -d -c 0-9`
			#check for versions less than or equal to 9
			firstchar=${pgVesrion:0:1}
			status=$?
			if [ $status -ne 0 ];then
				echo "Installation failed at step1"
				exit
			fi
			if [ "$firstchar" -ne "1" ]; then
				echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
				exit
			fi
			#check for versions lessthan than or equal to 10
			first2chars=${pgVesrion:0:2}
			if [ "$first2chars" -lt "11" ]; then
				echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
				exit
			fi

			#Add 
			sudo sed -i "275i shared_preload_libraries = 'timescaledb'" /etc/postgresql/11/main/postgresql.conf
			sudo service postgresql restart	

			d=$(date +%Y%m%d_%H%M%S)
			#echo "$d"
			#Renaming plc_data to plc_data_datetime
			if sudo -u postgres psql -l | grep '^ plc_data\b' > /dev/null ; then
		  	   sudo -u postgres psql -c 'ALTER DATABASE plc_data RENAME to plc_data_'$d''
			  status=$?
			  if [ $status -ne 0 ];then
			    echo "Unable to rename the database"
			    exit
			  fi
			fi

			echo "Step 2/11 Creating new database as plc_data"
			sudo -u postgres psql -c 'CREATE DATABASE plc_data;'
			status=$?
			if [ $status -ne 0 ];then
			   echo "Installation failed at step2"
			   exit
			fi

			echo "Step 3/11 Creating timescaledb extension on plc_data"
			sudo -u postgres psql -d 'plc_data' -c 'CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;'
			status=$?
			if [ $status -ne 0 ];then
			   echo "Installation failed at step3"
			   exit
			fi

			if [ "$dir" != "/" ]; then
				if [ ! -d $dir/pgdata ]; then
			   		sudo mkdir $dir/pgdata
				fi
		        sudo chown postgres:postgres $dir/pgdata
		        sudo chmod 700 $dir/pgdata
		        sudo rsync -av /var/lib/postgresql/11/main/ $dir/pgdata/data > pg_data_directory_log.txt
		        SRC=/var/lib/postgresql/11/main
				DST=$dir/pgdata/data
				sudo sed -i "s|$SRC|$DST|" /etc/postgresql/11/main/postgresql.conf
		        sudo service postgresql restart
		    fi

			echo "Step 4/11 Reading AWL file to create analog tables i.e., field_defs and data_table"
			sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'postgres';"
			#read -p "Kindly enter the file name to create analog tables in database: " fname
			# read -p "Kindly enter the path of the file: " fpath
			# if [ ! -f $fpath/$fname ]; then
			#    echo "Unable to locate file"
			#    exit
			# fi
			./dbdc -f CART.AWL -P 5432
			status=$?
			if [ $status -ne 0 ];then
			   echo "Unable to create tables in database"
			   exit
			fi

			echo "Create a view for data_table"
			sudo -u postgres psql -d 'plc_data' -c "create or replace view data_view as select * from data_table;"
			sudo -u postgres psql -d 'plc_data' -c "CREATE TABLE alarm_defs(paramnameFirst VARCHAR(100),alarm_id INT DEFAULT 0,paramnameSecond VARCHAR(100),db_field_name VARCHAR(20),bit_number INT DEFAULT 0,field_type VARCHAR(5));"
			break;;
			Exit )
			exit
			break;;
		   esac
		done
	else
		echo "Step 1/11 Installation of postgresql-11 and timescaledb."
		sudo dpkg -i *.deb
		status=$?
		if [ $status -ne 0 ];then
			echo "Installation failed at step1"
			exit
		fi
		pgVesrion=$( sudo -u postgres psql -c "SHOW server_version;" )
		pgVesrion=`echo $pgVesrion | tr -dc '[:alnum:]\n\r' | tr '[:upper:]' '[:lower:]'`
		pgVesrion=`echo $pgVesrion | tr -d -c 0-9`
		#check for versions less than or equal to 9
		firstchar=${pgVesrion:0:1}
		status=$?
		if [ $status -ne 0 ];then
			echo "Installation failed at step1"
			exit
		fi
		if [ "$firstchar" -ne "1" ]; then
			echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
			exit
		fi
		#check for versions lessthan than or equal to 10
		first2chars=${pgVesrion:0:2}
		if [ "$first2chars" -lt "11" ]; then
			echo "Your postgres version was not up to date.A clean installation of postgresql required in order to run our application. Please uninstall postgresql to set up plcdatacollector";
			exit
		fi

		#Add 
		sudo sed -i "275i shared_preload_libraries = 'timescaledb'" /etc/postgresql/11/main/postgresql.conf
		sudo service postgresql restart	
		
		d=$(date +%Y%m%d_%H%M%S)
		#echo "$d"
		#Renaming plc_data to plc_data_datetime
		if sudo -u postgres psql -l | grep '^ plc_data\b' > /dev/null ; then
	  	  sudo -u postgres psql -c 'ALTER DATABASE plc_data RENAME to plc_data_'$d''
		  status=$?
		  if [ $status -ne 0 ];then
		    echo "Unable to rename the database"
		    exit
		  fi
		fi

		echo "Step 2/11 Creating new database as plc_data"
		sudo -u postgres psql -c 'CREATE DATABASE plc_data;'
		status=$?
		if [ $status -ne 0 ];then
		   echo "Installation failed at step2"
		   exit
		fi

		echo "Step 3/11 Creating timescaledb extension on plc_data"
		sudo -u postgres psql -d 'plc_data' -c 'CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;'
		status=$?
		if [ $status -ne 0 ];then
		   echo "Installation failed at step3"
		   exit
		fi

		if [ "$dir" != "/" ]; then
			if [ ! -d $dir/pgdata ]; then
		   		sudo mkdir $dir/pgdata
			fi
	        sudo chown postgres:postgres $dir/pgdata
	        sudo chmod 700 $dir/pgdata
	        sudo rsync -av /var/lib/postgresql/11/main/ $dir/pgdata/data > pg_data_directory_log.txt
	        SRC=/var/lib/postgresql/11/main
			DST=$dir/pgdata/data
			sudo sed -i "s|$SRC|$DST|" /etc/postgresql/11/main/postgresql.conf
	        sudo service postgresql restart
	    fi

		echo "Step 4/11 Reading AWL file to create analog tables i.e., field_defs and data_table"
		sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'postgres';"
		#read -p "Kindly enter the file name to create analog tables in database: " fname
		# read -p "Kindly enter the path of the file: " fpath
		# if [ ! -f $fpath/$fname ]; then
		#    echo "Unable to locate file"
		#    exit
		# fi
		./dbdc -f CART.AWL -P 5432
		status=$?
		if [ $status -ne 0 ];then
		   echo "Unable to create tables in database"
		   exit
		fi

		echo "Create a view for data_table"
		sudo -u postgres psql -d 'plc_data' -c "create or replace view data_view as select * from data_table;"
		sudo -u postgres psql -d 'plc_data' -c "CREATE TABLE alarm_defs(paramnameFirst VARCHAR(100),alarm_id INT DEFAULT 0,paramnameSecond VARCHAR(100),db_field_name VARCHAR(20),bit_number INT DEFAULT 0,field_type VARCHAR(5));"
	fi

	echo "firewall reload at /etc/firewalld/firewalld.conf"

	echo "giving permission to /etc/firewalld folder"
	sudo chmod 777 /etc/firewalld

	echo "giving permission to firewalld.conf file"
	sudo chmod 777 /etc/firewalld/firewalld.conf

	res=$(sudo awk '/IndividualCalls=/ {print}' /etc/firewalld/firewalld.conf)
	
	if [ $res == 'IndividualCalls=no' ]; then
			echo "In firewalld.conf, IndividualCalls=no need to change to IndividualCalls=yes"
			sudo sed -i 's/IndividualCalls=no/IndividualCalls=yes/' /etc/firewalld/firewalld.conf
			status=$?
			if [ $status -ne 0 ];then
				echo "Not able to change the IndividualCalls status"
				else
				echo "In firewalld.conf set IndividualCalls=yes successfully"
			fi	
		else
			echo "In firewalld.conf IndividualCalls=yes already set"

	fi	

	echo "Step 5/11 Set up Configuration file for Data Logger API"
	#Create a folder as dataloggerapi in /etc/
	if [ ! -d /etc/dataloggerapi ]; then
	   sudo mkdir /etc/dataloggerapi
	fi

	#Create a folder as versionInfo in /bin/
	if [ ! -d /bin/versionInfo ]; then
		sudo mkdir /bin/versionInfo
	fi	

	#Move versionInfo exe file to /bin/versionInfo
	sudo cp versionAPI /bin/versionInfo
	status=$?
	if [ $status -ne 0 ]; then
		echo "Installation failed at Step 5A"
		exit
	fi
		
	#Move dataloggerapi configuration file to /etc/plcdatacollector
	sudo cp dataloggerapi.conf /etc/dataloggerapi
    status=$?
    if [ $status -ne 0 ];then
        echo "Installation failed at step5"
    	exit
    fi
	
	#Create a folder as dataloggerapi in /bin/
	echo "Step 6/11 Create, Enable and Start the API Service"
	if [ ! -d /bin/dataloggerapi ]; then
	    sudo mkdir /bin/dataloggerapi
	fi
    sudo cp dataloggerapi /bin/dataloggerapi
    status=$?
    if [ $status -ne 0 ];then
   	echo "Installation failed at step6"
   	exit
    fi
    #sudo cp dataloggerapi.service /etc/systemd/system
    sudo systemctl daemon-reload
    sudo systemctl enable $currpath/dataloggerapi.service
    sudo systemctl start dataloggerapi.service
	
	#Need user input to provide plc IP address and port
	read -p "Kindly enter the port No of plc: " prt

	echo "Step 7/11 Set up Configuration file for PLC Data Collector"
	#Create a folder as plcdatacollector in /etc/
	if [ ! -d /etc/plcdatacollector ]; then
	   sudo mkdir /etc/plcdatacollector
	fi

	#Move plcdatacollector configuration file to /etc/plcdatacollector
	sudo cp plcdatacollector.conf /etc/plcdatacollector
	#Replace the port of plcsimulator in plcdatacollector configuration file with user entered port
	sudo sed -i 's/2051/'$prt'/' /etc/plcdatacollector/plcdatacollector.conf
	sudo firewall-cmd --zone=public --permanent --add-port=$prt/udp
	sudo firewall-cmd --reload
	status=$?
	if [ $status -ne 0 ];then
	  echo "Installation failed at step7"
	  exit
	fi
	

	sudo -u postgres psql -d 'plc_data' -c "SELECT set_chunk_time_interval('data_table', INTERVAL '24 hours');"
	echo "Step 8/11 Create, Enable and Start the plcdatacollector Service"
	status=$?
	if [ $status -ne 0 ];then
	   echo "Installation failed at step8"
	   exit
	fi
	if [ ! -d /bin/plcdatacollector ]; then
	    sudo mkdir /bin/plcdatacollector
	fi
	if [ ! -d /etc/versionInfo ]; then
    	sudo mkdir /etc/versionInfo
    fi
	sudo cp versionconfig.conf /etc/versionInfo
	sudo cp plcdatacollector /bin/plcdatacollector
	status=$?
	if [ $status -ne 0 ];then
		echo "Installation failed at step8"
		exit
	fi 
	#sudo cp plcdatacollector.service /etc/systemd/system
	sudo systemctl daemon-reload 
	sudo systemctl enable $currpath/plcdatacollector.service
	sudo systemctl start plcdatacollector.service


	echo "Step 9/11 Enable retention policy to remove the data which is older than 7 days for data_table"
	if [ "$dir" != "/" ]; then
		FREE=`df $dir | awk '{ print $4 }' | tail -n 1| cut -d'%' -f1`
	else
		FREE=`df -k --output=avail "$PWD" | tail -n1` 
	fi

	read -p "Please enter how many hours of data you want to store in database per day eg:8/16/24 (1-24) : " hrs
	if [ $hrs -lt 25 ];then
		echo $hrs | bc
			read -p "Please enter frequency of data you want to store in database (miliseconds) eg:10/20/100 (10-100) : " freq
			if [ $freq -lt 100 ];then
				echo $freq 
				else
	     		echo "Please enter frequency ranging from(10,20,50...100)"
	     		exit
	     	fi
		else
     	echo "Please enter hours ranging from(1-24)"
     	exit
	fi

	#remaining=`echo $((FREE - 5242880)) | bc` #5242880 value is in kilobytes(-k option returns values in kb)
	remaining=`echo $(((FREE*80)/100))| bc` #using only 80 % memory from free disc space

	echo "calculate records/hours with respect to user provided frequency "
	records=`echo $((3600000 / freq))| bc`
	echo $records
	
	reqmem_data=`echo $(((records * 1552)/1000))| bc`    #1552 bytes required per record for data_table according to memory calculation
	echo "required memory for 1 hour and  provided frequency in miliseconds for data_table"
	echo $reqmem_data
	tocheck_data=`echo $(((remaining) / (reqmem_data * hrs))) | bc` #If data stored in database as 8 hrs/day for every 50 ms
	echo "tocheck_data value :"
	echo $tocheck_data

	read -p "Kindly enter the number of days of older data to keep in the database: " ndays
	
	if [ $ndays -lt $tocheck_data ];then
		sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('data_table', INTERVAL '$ndays days');"
		status=$?
		if [ $status -ne 0 ];then
		   echo "Unable to add retention policy"
		   exit
		fi
		echo "Enabled retention policy to remove the data which is older than $ndays days from data_table"
	else
     	echo "The suggested number of days "$tocheck
     	sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('data_table', INTERVAL '$ndays days');"
    	status=$?
		if [ $status -ne 0 ];then
		   echo "Unable to add retention policy"
		   exit
		fi
		echo "Enabled retention policy to remove the data which is older than $ndays days from data_table"
	fi

	echo "Do you wish to enable compression policy to compress the data for data_table?"
	select yn in "Yes" "No"; do
		case $yn in
	        Yes )
		sudo -u postgres psql -d 'plc_data' -c "ALTER TABLE data_table SET (timescaledb.compress,timescaledb.compress_segmentby = '\"comp_ref\"',timescaledb.compress_orderby = 'insert_time_stamp DESC');"
		sudo -u postgres psql -d 'plc_data' -c "SELECT add_compression_policy('data_table', INTERVAL '1 days');"
		break;;
	        No ) 
		echo "Data Compression Policy Skipped"
		break;;
	   esac
	done

	echo "Step 10/11 Enable port access"
	sudo firewall-cmd --zone=public --permanent --add-port=6789/tcp
	sudo firewall-cmd --reload

    echo "Step 11/11 Create, Enable and Start the commissioning-server Service"
    sudo firewall-cmd --zone=public --permanent --add-port=5433/tcp
    sudo firewall-cmd --reload
    if [ ! -d /bin/commissioning-server ]; then
    	sudo mkdir /bin/commissioning-server
    fi
	sudo cp commissioning-server /bin/commissioning-server
	status=$?
	if [ $status -ne 0 ];then
		echo "Installation failed at step11"
		exit
	fi
	#sudo cp commissioning-server.service /etc/systemd/system
	sudo systemctl daemon-reload
	sudo systemctl enable $currpath/commissioning-server.service
	sudo systemctl start commissioning-server.service
    
	echo "The installation process is successful. Kindly follow below steps to check everything working or not."
	echo "Kindly check plcdatacollector & dataloggerapi service(s) are running or not. Use service dataloggerapi status AND service plcdatacollector status to check the same."
	echo "Check the data in data_table, if the data is inserting or not. The row count will be increased everytime if you execute the query select count(*) from data_table"
	fi
	break;
	done
	break;;
        No ) exit;;
    esac
done
echo "Do you wish to setup KPI data collection and it's API service?"
select yn in "Yes" "No"; do
	case $yn in
        Yes )
	./cdcreate
	status=$?
	if [ $status -ne 0 ];then
	   echo "Unable to create KPI tables in database"
	   exit
	fi
        echo "Create, Enable and Start the KPI Logger API Service"
	if [ ! -d /bin/kpiloggerapi ]; then
	    sudo mkdir /bin/kpiloggerapi
	fi
	    sudo cp kpiloggerapi /bin/kpiloggerapi
	    status=$?
	    if [ $status -ne 0 ];then
	   	echo "Installation failed at step6"
	   	exit
	    fi
	    #sudo cp kpiloggerapi.service /etc/systemd/system
	    sudo systemctl daemon-reload 
	    sudo systemctl enable $currpath/kpiloggerapi.service
	    sudo systemctl start kpiloggerapi.service
	
	#Create a folder as kpidatacollector in /etc/
	if [ ! -d /etc/kpidatacollector ]; then
	   sudo mkdir /etc/kpidatacollector
	fi
	sudo cp kpidatacollectioncron /etc/kpidatacollector/kpidatacollectioncron
	sudo sed -i "22i 10 */1 * *  * root  /etc/kpidatacollector/kpidatacollectioncron" /etc/crontab
	sudo service cron restart
	
	echo "Enable port access"
	sudo firewall-cmd --zone=public --permanent --add-port=9876/tcp
	sudo firewall-cmd --reload
	echo "The KPI Data Collection was enabled successfully. Kindly follow below steps to check if it's working or not."
	echo "Kindly create at least one KPI and check data in kpi_value_table after every hour 10 minutes."
	echo "Kindly check kpiloggerapi was running or not. Use service kpiloggerapi status to check the same."
	break;;
        No ) 
	echo "KPI Data Collection Skipped"
	break;;
   esac
done
echo "Do you wish to setup alarm data collection and it's API service?"
select yn in "Yes" "No"; do
	case $yn in
        Yes )
	echo "How many alarms do you want to setup in databse?"
	select yn in "640" "1280"; do
		case $yn in
	    	640 )
		./cdalrm 640
		break;;
			1280 )
		./cdalrm 1280
		break;;
	    esac
	done		
	status=$?
	if [ $status -ne 0 ];then
		echo "Unable to create alarm tables in database"
		exit
	fi

	sudo -u postgres psql -d 'plc_data' -c "SELECT remove_retention_policy('data_table')";
	status=$?
	if [ $status -ne 0 ];then
	   echo "Unable to remove retention policy"
	   exit
	fi
	echo "older retention policy removed successfully"

	read -p "Please enter alarm_data confugrations you have selected to configure alarm_table (640/1280): " alarms
	echo $alarms

	if [ $alarms -eq 640 ];then
		#factor = 1552/272= 5.70588==~6   using approx 6:1
		#6.70588-------1
		#100-----------?(149122)/(100*10000)

		#memforalarms_640=`echo $((remaining / 7))| bc`
		memforalarms_640=`echo $(((remaining * 149122)/(100*10000)))| bc`
		echo "memory for memforalarms_640 :"
		echo $memforalarms_640
		memfordata=`echo $((remaining - memforalarms_640))| bc`
		#memfordata=`echo $((remaining - ((remaining * 149122)/(100*10000))))| bc`
		echo "memory for data :"
		echo $memfordata

		reqmem_data=`echo $(((records * 1552)/1000))| bc`    #1552 bytes required per record for data_table according to memory calculation
		echo "required memory for 1 hour and  provided frequency in miliseconds for data_table"
		echo $reqmem_data
		tocheck_data=`echo $(((memfordata) / (reqmem_data * hrs))) | bc` #If data stored in database as 8 hrs/day for every 50 ms
		echo "tocheck_data value :"
		echo $tocheck_data

		read -p "Kindly enter the number of days of older data into data_table to keep in the database: " ndays
		if [ $ndays -lt $tocheck_data ];then
			sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('data_table', INTERVAL '$ndays days')";
			status=$?
			if [ $status -ne 0 ];then
			   echo "Unable to add retention policy"
			   exit
			fi
			echo "Enabled retention policy to remove the data which is older than $ndays days from data_table"
		else
	     	echo "The suggested number of days "$tocheck_data
	     	read -p "Kindly enter the number of days of older data to keep in the database less than suggested number of days: " ndays
	     	sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('data_table', INTERVAL '$ndays days')"; 
	     	status=$?
			if [ $status -ne 0 ];then
			   echo "Unable to add retention policy"
			   exit
			fi
			echo "Enabled retention policy to remove the data which is older than $ndays days from data_table"	
		fi

		reqmem_alarms_640=`echo $(((records * 272)/1000))| bc`  #272 bytes required per record for alarm_table(640 alarms) according to memory calculation
		echo "required memory for 1 hour and provided frequency in miliseconds for alarms 640"
		echo $reqmem_alarms_640

		tocheck_alarms_640=`echo $(((memforalarms_640) / (reqmem_alarms_640 * hrs))) | bc` #If data stored in database as 8 hrs/day for every 50 ms
		echo "tocheck_alarms_640 value :"
		echo $tocheck_alarms_640

		read -p "Kindly enter the number of days of older data into data_table to keep in the database: " ndays
		if [ $ndays -lt $tocheck_alarms_640 ];then
			sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('alarm_table', INTERVAL '$ndays days')";
			status=$?
			if [ $status -ne 0 ];then
			   echo "Unable to add retention policy"
			   exit
			fi
			echo "Enabled retention policy to remove the data which is older than $ndays days from alarm_table"
		else
	     	echo "The suggested number of days "$tocheck_alarms_640
	     	read -p "Kindly enter the number of days of older data to keep in the database less than suggested number of days: " ndays
	     	sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('alarm_table', INTERVAL '$ndays days')"; 
	     	status=$?
			if [ $status -ne 0 ];then
			   echo "Unable to add retention policy"
			   exit
			fi
			echo "Enabled retention policy to remove the data which is older than $ndays days from alarm_table"	
		fi

		else
		if [ $alarms -eq 1280 ];then
			#5.40909091-------1
			#100--------------?(184874)/(100*10000)

			#memforalarms_1280=`echo $((remaining / 7))| bc`
			memforalarms_1280=`echo $(((remaining * 184874)/(100*10000)))| bc`
			echo $memforalarms_1280
			memfordata=`echo $((remaining - memforalarms_1280))| bc`
			echo $memfordata

			reqmem_data=`echo $(((records * 1552)/1000))| bc`    #1552 bytes required per record for data_table according to memory calculation
			echo "required memory for 1 hour and  provided frequency in miliseconds for data_table"
			echo $reqmem_data
			tocheck_data=`echo $(((memfordata) / (reqmem_data * hrs))) | bc` #If data stored in database as 8 hrs/day for every 50 ms
			echo "tocheck_data value :"
			echo $tocheck_data

			read -p "Kindly enter the number of days of older data into data_table to keep in the database: " ndays
			if [ $ndays -lt $tocheck_data ];then
				sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('data_table', INTERVAL '$ndays days')";
			else
		     	echo "The suggested number of days "$tocheck_data
		     	read -p "Kindly enter the number of days of older data to keep in the database less than suggested number of days: " ndays
		     	sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('data_table', INTERVAL '$ndays days')";
		     	status=$?
				if [ $status -ne 0 ];then
				   echo "Unable to add retention policy"
				   exit
				fi
				echo "Enabled retention policy to remove the data which is older than $ndays days from data_table" 	
			fi
			reqmem_alarms_1280=`echo $(((records * 352)/1000))| bc`  #272 bytes required per record for alarm_table(640 alarms) according to memory calculation
			echo "required memory for 1 hour and provided frequency in miliseconds for alarms 640"
			echo $reqmem_alarms_1280

			tocheck_alarms_1280=`echo $(((memforalarms_1280) / (reqmem_alarms_1280 * hrs))) | bc` #If data stored in database as 8 hrs/day for every 50 ms
			echo "tocheck_alarms_640 value :"
			echo $tocheck_alarms_1280

			read -p "Kindly enter the number of days of older data into data_table to keep in the database: " ndays
			if [ $ndays -lt $tocheck_alarms_1280 ];then
				sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('alarm_table', INTERVAL '$ndays days')";
				status=$?
				if [ $status -ne 0 ];then
				   echo "Unable to add retention policy"
				   exit
				fi
				echo "Enabled retention policy to remove the data which is older than $ndays days from alarm_table"

			else
		     	echo "The suggested number of days "$tocheck_alarms_1280
		     	read -p "Kindly enter the number of days of older data to keep in the database less than suggested number of days: " ndays
		     	sudo -u postgres psql -d 'plc_data' -c "SELECT add_retention_policy('data_table', INTERVAL '$ndays days')"; 
		     	status=$?
				if [ $status -ne 0 ];then
				   echo "Unable to add retention policy"
				   exit
				fi
				echo "Enabled retention policy to remove the data which is older than $ndays days from alarm_table"	
			fi
			else
 			echo "failed at retantion policy"
 			exit
 		fi
	fi

	sudo -u postgres psql -d 'plc_data' -c "create or replace view alarm_view as select * from alarm_table;"
	status=$?
	if [ $status -ne 0 ];then
	   echo "Unable to add retention policy"
	   exit
	fi
	echo "alarm_table view created successfully"
	
	echo "Do you wish to enable compression policy to compress the data for alarm_table?"
	select yn in "Yes" "No"; do
		case $yn in
	        Yes )
		sudo -u postgres psql -d 'plc_data' -c "ALTER TABLE alarm_table SET (timescaledb.compress,timescaledb.compress_segmentby = '\"comp_ref\"',timescaledb.compress_orderby = 'insert_time_stamp DESC');"
		if [ $status -ne 0 ];then
	   		echo "Unable to alter alarm_table"
	   		exit
		fi
		sudo -u postgres psql -d 'plc_data' -c "SELECT add_compression_policy('alarm_table', INTERVAL '1 days');"
		if [ $status -ne 0 ];then
	   		echo "Unable to add compression policy"
	   		exit
		fi		
		break;;
	        No ) 
		echo "Data Compression Policy Skipped"
		break;;
	   esac
	done

	echo "Create, Enable and Start the alarmdatacollector Service"
	sudo -u postgres psql -d 'plc_data' -c "SELECT set_chunk_time_interval('alarm_table', INTERVAL '24 hours');"
	read -p "Kindly enter the port No of alarm data: " alarmprt
	sudo sed -i 's/2061/'$alarmprt'/' /etc/plcdatacollector/plcdatacollector.conf
	sudo firewall-cmd --zone=public --permanent --add-port=$alarmprt/udp
	sudo firewall-cmd --reload
	status=$?
	if [ $status -ne 0 ];then
	   echo "Installation failed to setup alarm data collection"
	   exit
	fi
	if [ ! -d /bin/alarmdatacollector ]; then
	    sudo mkdir /bin/alarmdatacollector
	fi
	    sudo cp alarmdatacollector /bin/alarmdatacollector
	    status=$?
	    if [ $status -ne 0 ];then
	   	echo "Installation failed to setup alarm data collection"
	   	exit
	    fi 
	    #sudo cp alarmdatacollector.service /etc/systemd/system
	    sudo systemctl daemon-reload 
	    sudo systemctl enable $currpath/alarmdatacollector.service
	    sudo systemctl start alarmdatacollector.service

	sudo sync	

	echo "The alarm data collection was enabled successfully. Kindly follow below steps to check if it's working or not."
	echo "Kindly check alarmdatacollector service is running or not. Use service alarmdatacollector status to check the same."
	echo "Check the data in alarm_table, if the data is inserting or not. The row count will be increased everytime if you execute the query select count(*) from alarm_table"

	break;;
        No ) exit;;
    esac
done




