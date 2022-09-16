sudo mkdir /var/lib/node_exporter
sudo mkdir /var/lib/node_exporter/textfile_collector
sudo chown -R $(id -u):$(id -g) /var/lib/node_exporter/textfile_collector
docker stop prom_exporter
docker rm prom_exporter
docker run -d \
  --name="prom_exporter" \
  --net="host" \
  --pid="host" \
  -v "/:/host:ro,rslave" \
  quay.io/prometheus/node-exporter:latest \
  --path.rootfs=/host \
  --web.disable-exporter-metrics \
  --collector.disable-defaults \
  --collector.textfile \
  --collector.textfile.directory=/host/var/lib/node_exporter/textfile_collector
docker ps
