# Najan Dictionary Viewer and Editor

CRUD app for the dictionary of the Najan conlang, using [httplib](https://github.com/yhirose/cpp-httplib) and SQLite for the backend and HTML + JS for UI.

(Eventual) feature list:
- Create an empty dictionary schema or import from SQLite DB file
- Add/delete English/Najan word classes
- Add/delete/modify English/Najan words and definitions
- Define English and Najan translations (many to many)
- Search for words by regex, filter by class, etc.
- Generate list of dictionary entries as LaTeX markup
