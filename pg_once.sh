echo "#pragma once\n" > /tmp/head.txt
for file in $(find . -name "*.h"); do
    cat /tmp/head.txt $file > $file.modified
    mv $file.modified $file
done
