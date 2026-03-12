#!/bin/zsh

/Users/fantang/local/ice.3.7.9/debug/bin/slice2py -I. --output-dir ../../layer_app/QIBTrader_Pro/ice QStruc.ice
/Users/fantang/local/ice.3.7.9/debug/bin/slice2py -I. --output-dir ../../layer_app/QIBTrader_Pro/ice QDatabase.ice

echo "Done!"
