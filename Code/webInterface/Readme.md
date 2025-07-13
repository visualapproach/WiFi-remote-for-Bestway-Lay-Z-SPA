# Web Interface

The web interface gets automatically build when the filesystem is created.

## Templating

The template engine is very simple and built loosely on the Django syntax.

### Layouts

Layouts define the common ground for each page and can be including with `{% extends "../path/to/layout.html" %}`

### JavaScript Files

JavaScript files can be injected directly into the HTML using the `embed_scripts()` function inside templates.

The base layout defines this in the block `embed_scripts_default`:

```django
{% block embed_scripts_default %}
{% run embed_scripts('index', 'function', 'i18n', 'darkmode', 'remove') %}
{% endblock %}
```
