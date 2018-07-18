#! /bin/sh
# This script in fetching the latest version of libtomcrypt and libtommath
# projects and create *.orig files to make it easier to compare them with
# the version of libtomcrypt and libtommath used in this project.
# The indentation in the NCR source tree is better than the one used in the
# original libtom project. However, we may switch back to the original project
# indentation to make the changes done by this project more clear.

LT_URL=https://github.com/libtom
LTC=libtomcrypt
LTM=libtommath
LTS=libtom_snapshots
CURL=$(which curl)
GIT=$(which git)
PROJ_DIR=$(dirname $0 | sed "s,^\([^/]\),$(pwd)/\1,")/..

die() {
	[ "$*" != "" ] && echo $*
	cd ${PROJ_DIR}
	rm -Rf ${LTS}
	echo "Aborting"
	exit 1
}

cp_file() {
	echo "  ${file}"
}

[ "$CURL" = "" -a "$GIT" = "" ] \
	&& die "Please install git or curl to use this script"

mkdir ${PROJ_DIR}/${LTS}
cd ${PROJ_DIR}/${LTS}

# Get a snapshot of libtomcrypt and libtommath
for libtom in ${LTC} ${LTM}; do
	if [ "$CURL" != "" ]; then
		url=${LT_URL}/${libtom}/archive/master.tar.gz
		echo -n "Downloading ${libtom} snapshot here: ${url}... "
		mkdir ${libtom}
		$CURL -sL ${url} | tar xz -C ${libtom} --strip-components=1 \
			&& echo "Ok" && continue || echo "Failed"
		rmdir ${libtom}
	fi
	if [ "$GIT" != "" ]; then
		url=${LT_URL}/${libtom}
		echo -n "Cloning the ${libtom} git tree here: ${url}... "
		$GIT clone -q ${url} && echo "Ok" && continue || die "Failed"
	fi
	die
done

cd ..

for libtom in ${LTC} ${LTM}; do
	echo "Updating ${libtom}"
	# Interate on each libtom file...
	find ${libtom} -type f | while read file; do
		echo -n "  ${file} "
		# ... and look for a matching file in latest libtom snapshots
		dest=$(find ${LTS}/${libtom} -type f -name ${file##*/} \
			| head -1)
		[ "${dest}" != "" ] \
			&& ! cmp -s ${file} ${dest} \
			&& cp ${dest} ${file}.orig \
			&& echo -n "(Updated)"
		echo
	done
done

rm -Rf ${LTS}

# Then you want to do something like this
#for file in $(git status | sed -n 's/^[[:blank:]]*libtom/libtom/p' | grep -v "done$"); do diff -b $file ${file%.*}; echo; echo -n "Accept modifications for file ${file%.*}? (y/n) "; read ans; [ "$ans" = "y" ] && cp $file ${file%.*} && mv $file ${file}.done; printf "\n\n\n\n\n"; done

