#!/bin/bash
echo "Actualizando códigos de bench-q en Olivo"
rsync -avli * olivo:~/bench-q/
