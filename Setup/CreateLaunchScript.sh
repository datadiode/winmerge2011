if [ "$(id -u)" != "0" ]; then
	trap '' 2
	echo "Creating WinMerge launch script in /usr/local/bin."
	sudo -p "Please enter root password: " sh "$0" "$1"
	rm "$0"
	exit
fi
echo "#!/bin/sh" > /usr/local/bin/WinMerge
echo "wine \"$1\" \${WINMERGE_OPTIONS} \$* 2> /dev/null" >> /usr/local/bin/WinMerge
chmod +x /usr/local/bin/WinMerge
