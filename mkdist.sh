#!/bin/sh

BASE=/home/piers/git/public/python-sapnwrfc

cd $BASE
python setup.py clean
python setup.py sdist
cd dist
chmod -R a+r *z
rsync -av --delete *z piersharding.com:www/download/python/sapnwrfc/


cd $BASE
mkdir $BASE/tmp
chmod -R a+r doc


rsync -av --delete doc tmp/
cd $BASE/tmp/doc
rm -rf tools *.log *.tex Makefile *.how *.ind *.l2h *~
find . -type d -name CVS -exec rm -rf {} \;
cd $BASE/tmp
rsync -av --delete doc piersharding.com:www/download/python/sapnwrfc/
