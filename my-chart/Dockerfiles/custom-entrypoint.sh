#!/bin/bash
# Script para modificar el index.html con información distintiva

# Captura la hora de inicio y la dirección IP
START_TIME=$(date)
IP_ADDRESS=$(hostname -I | cut -d' ' -f1)

# Escribe la nueva información en index.html
echo "<html><body><h1>Apache Server at ${START_TIME}</h1><h2>IP Address: ${IP_ADDRESS}</h2></body></html>" > /usr/local/apache2/htdocs/index.html

# Ejecutar Apache en primer plano
exec httpd-foreground
