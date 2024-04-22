# Usar la imagen base de Apache
FROM httpd:latest

# Instalar curl y otros paquetes útiles (si necesitas más)
RUN apt update -y && \
    apt install curl -y && \
    rm -rf /var/lib/apt/lists/*

# Copiar el script de inicio personalizado
COPY custom-entrypoint.sh /usr/local/bin/

# Dar permisos de ejecución al script
RUN chmod +x /usr/local/bin/custom-entrypoint.sh

# Configurar el script como punto de entrada
ENTRYPOINT ["custom-entrypoint.sh"]
CMD ["httpd-foreground"]
