#!/bin/sh

#for some reason gumstix $HOSTNAME not getting set automatically
# can't make it work per-context yet... requires editing:

set $INTERFACE_HOST = "131.179.141.19" #GUMSTIX
set $CONTROLLER_HOST = "borges.metwi.ucla.edu"


echo "This script assumes $INTERFACE_HOST  is the lighting controller"
echo "				  and $CONTROLLER_HOST  is the lighting controller"