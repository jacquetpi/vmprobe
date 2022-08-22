ssh pijacque@spirals-tornado.lille.inria.fr "cd /usr/local/src/vmprobe/ && rm -r src/*"
scp -r src/* pijacque@spirals-tornado.lille.inria.fr:/usr/local/src/vmprobe/src
ssh pijacque@spirals-tornado.lille.inria.fr "cd /usr/local/src/vmprobe/ && ./compil.sh"
