# Incident {{ incident.number }} — {{ incident.title | escape }}

{% set visible_events = events %}
Status: {% if incident.resolved %}resolved{% elif incident.mitigated %}mitigated{% else %}investigating{% endif %}
Owner: {{ incident.owner.name }} ({{ incident.owner.team }})

## Timeline

{% for event in visible_events %}
{{ loop.index }}. **{{ event.time }}** — {{ event.summary | escape }}
   {% if event.details %}{{ event.details | trim }}{% else %}No additional details.{% endif %}
{% endfor %}

## Affected services

{% for service, details in affected_services %}
- `{{ service }}`: {{ details.status }}
  {% for region in details.regions %}
  - {{ region }}{% if loop.last %}.{% else %};{% endif %}
  {% endfor %}
{% endfor %}

## Labels

{% for key, value in labels %}- {{ key }} = {{ value }}
{% endfor %}

Generated for {{ account.name }} · {{ incident.generated_at }}
