# path sha256 records groups
{% for row in rows %}{{ row.path }} {{ row.checksum }} {{ row.records }} {{ row.groups }}
{% endfor %}