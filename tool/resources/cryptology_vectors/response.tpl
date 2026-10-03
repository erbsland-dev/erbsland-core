{% for line in header %}{{ line }}
{% endfor %}
{% for group in groups %}{% for field in group.headers %}[{{ field.name }} = {{ field.value }}]
{% endfor %}
{% for record in group.records %}{% for field in record %}{{ field.name }} = {{ field.value }}
{% endfor %}
{% endfor %}{% endfor %}
