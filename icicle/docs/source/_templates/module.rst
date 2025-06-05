{{ name | escape | underline}}

.. currentmodule:: {{ fullname }}

{{cli_functions}}

{{all_cli_functions}}

.. automodule:: {{ fullname }}
   :ignore-module-all:

   {% block attributes %}
   {% if attributes %}
   .. rubric:: Module Attributes

   .. autosummary::
   {% for item in attributes %}
      ~{{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}

   {% block functions %}
   {% if functions %}
   .. rubric:: {{ _('Functions') }}

   .. autosummary::
   {% for item in functions %}
      ~{{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}

   {% block classes %}
   {% if classes %}
   .. rubric:: {{ _('Classes') }}

   .. autosummary::
   {% for item in classes %}
      ~{{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}

   {% block exceptions %}
   {% if exceptions %}
   .. rubric:: {{ _('Exceptions') }}

   .. autosummary::
   {% for item in exceptions %}
      ~{{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}

{% block modules %}
{% if modules %}
.. rubric:: Modules

.. autosummary::
   :toctree:
   :template: module.rst
   :recursive:
{% for item in modules %}
   {{ item }}
{%- endfor %}
{% endif %}
{% endblock %}

{% block functions_content %}
{% if functions %}

{% for item in functions %}

{{ item | escape | underline('-')  }}

.. autofunction:: {{ item }}

{%- endfor %}
{% endif %}
{% endblock %}

{% block commands_content %}
{% if commands %}

{% for item in commands %}

{{ item | escape | underline('-')  }}

.. autofunction:: {{ item }}

{%- endfor %}
{% endif %}
{% endblock %}

{% block classes_content %}
{% if classes %}

{% for item in classes %}

{{ item | escape | underline('-') }}

.. autoclass:: {{ item }}
   :members:
   :undoc-members:
   :show-inheritance:

   {% block c_methods %}
   .. automethod:: __init__

   {% if methods %}
   .. rubric:: {{ _('Methods') }}

   .. autosummary::
   {% for item in methods %}
      ~{{ name }}.{{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}

   {% block c_attributes %}
   {% if attributes %}
   .. rubric:: {{ _('Attributes') }}

   .. autosummary::
   {% for item in attributes %}
      ~{{ name }}.{{ item }}
   {%- endfor %}
   {% endif %}
   {% endblock %}
{%- endfor %}
{% endif %}
{% endblock %}

{% block exceptions_content %}
{% if exceptions %}

{% for item in exceptions %}

{{ item | escape | underline('-')  }}

.. autofunction:: {{ item }}

{%- endfor %}
{% endif %}
{% endblock %}
