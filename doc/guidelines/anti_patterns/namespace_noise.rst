***************
Namespace Noise
***************

:Rule ID: ``namespace_noise``
:Severity: low

When a domain prefix occurs more than three times in an implementation namespace, import that domain immediately inside
the namespace and remove redundant prefixes where name lookup remains unambiguous.
Count each domain independently: four different prefixes used once do not trigger this rule.
The shipped configuration selects the domains ``text``, ``unit``, ``err``, ``mem``, ``options``, ``path``, and
``stream``.

.. code-block:: cpp

    namespace erbsland::conf {

    using namespace text;
    using namespace unit;

    // Implementation using String, ByteIndex, and related domain names.

    }

Mechanical Detection
====================

Only ``.cpp`` files are checked.
The scanner emits one finding per domain and named namespace block with at least four uses.
It counts uses inside functions and classes belonging to that namespace, but keeps nested and reopened namespace blocks
separate because imports have lexical scope.
Headers, template implementation files, global scope, comments, literals, preprocessing directives, namespace headers,
and ``using`` declarations are excluded.
Platform conditional uses excluded by the scanner do not contribute to the count.
Prefixes qualified through ``el::`` and ``erbsland::`` count; unrelated leading qualifiers do not.
The optional ``Anti Patterns . Namespace Noise`` section configures ``Domains`` and ``Ignored Symbols``.
Both lists are empty when omitted.
Ignored symbols match complete domain-qualified identifiers and do not contribute to the threshold or reported location.
The shipped ``mem::secureErase`` and ``text::join`` exclusions retain qualifications needed to avoid collisions with
class members and functions in other domains.

This is a readability heuristic, not a name-lookup analysis.
Check collisions, namespace aliases, and intentional disambiguation before changing source.
Existing imports do not make repeated explicit qualifications necessary, so they do not suppress findings.
