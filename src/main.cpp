#include <fmt/format.h>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <sqlite_orm/sqlite_orm.h>

#include <regex>
#include <string>

using httplib::Request;
using httplib::Response;

namespace nl = nlohmann;

using namespace sqlite_orm;

struct naj_class {
	int id;
	std::string name;
};

struct eng_class {
	int id;
	std::string name;
};

struct naj {
	std::string lemma;
	int word_class;
	std::string def;
};

struct naj_note {
	std::string naj;
	std::string note;
};

struct eng {
	std::string lemma;
	int word_class;
};

struct trans {
	std::string naj;
	std::string eng;
	std::string note;
};

struct cf {
	std::string naj1;
	std::string naj2;
};

struct checked_handler {
	std::function<void(Request const&, Response&)> f;
	void operator ()(Request const& request, Response& response) {
		try {
			f(request, response);
			response.set_header("status", "200");
		} catch (std::exception& ex) {
			response.set_header("status", "500");
			response.set_content(fmt::format("Error: {}", ex.what()), "text/plain");
		}
	}
};

int main() {
	auto db = make_storage("dictionary.db",
		make_table("naj_classes",
			make_column("id", &naj_class::id, primary_key()),
			make_column("name", &naj_class::name)).without_rowid(),
		make_table("eng_classes",
			make_column("id", &eng_class::id, primary_key()),
			make_column("name", &eng_class::name)).without_rowid(),
		make_table("naj",
			make_column("lemma", &naj::lemma, primary_key()),
			make_column("class", &naj::word_class),
			make_column("def", &naj::def),
			foreign_key(&naj::word_class).references(&naj_class::id)).without_rowid(),
		make_table("naj_notes",
			make_column("naj", &naj_note::naj, primary_key()),
			make_column("note", &naj_note::note),
			foreign_key(&naj_note::naj).references(&naj::lemma)).without_rowid(),
		make_table("eng",
			make_column("lemma", &eng::lemma),
			make_column("class", &eng::word_class),
			primary_key(&eng::lemma, &eng::word_class),
			foreign_key(&eng::word_class).references(&eng_class::id)).without_rowid(),
		make_table("trans",
			make_column("naj", &trans::naj),
			make_column("eng", &trans::eng),
			make_column("note", &trans::note),
			primary_key(&trans::naj, &trans::eng),
			foreign_key(&trans::naj).references(&naj::lemma),
			foreign_key(&trans::eng).references(&eng::lemma)).without_rowid(),
		make_table("cf",
			make_column("naj1", &cf::naj1),
			make_column("naj2", &cf::naj2),
			primary_key(&cf::naj1, &cf::naj2),
			foreign_key(&cf::naj1).references(&naj::lemma),
			foreign_key(&cf::naj2).references(&naj::lemma)).without_rowid());
	db.sync_schema();

	httplib::Server server;
	if (!server.is_valid()) {
		fmt::print("server has an error...\n");
		return -1;
	}

	server.set_base_dir("./www");

	server.Get("/naj/(.*)",
		checked_handler{[&](Request const& request, Response& response) {
			nl::json content = nl::json::array();
			std::regex regex;
			try {
				regex = std::regex{request.matches[1].str()};
			} catch (std::regex_error) {
				// Invalid regex from client.
				regex = std::regex{""};
			}
			for (auto const& n : db.get_all<naj>()) {
				if (std::regex_search(n.lemma, regex)) {
					content.push_back({
						{"lemma", n.lemma},
						{"word_class", db.get<naj_class>(n.word_class).name},
						{"def", n.def}});
				}
			}
			fmt::print(content.dump());
			response.set_content(content.dump(), "text/json");
		}});

	server.Get("/naj-classes",
		checked_handler{[&](Request const& request, Response& response) {
			nl::json content = nl::json::object();
			for (auto const& nc : db.get_all<naj_class>()) {
				content[std::to_string(nc.id)] = nc.name;
			}
			response.set_content(content.dump(), "text/json");
		}});

	server.Post("/add/naj-class/(\\w+)",
		checked_handler{[&](Request const& request, Response& response) {
			std::string name = request.matches[2];
			db.insert(naj_class{db.count<naj_class>(), name});
		}});

	server.Get("/stop", [&](Request const&, Response&) { server.stop(); });

	server.set_error_handler(
		[](Request const&, Response& response) {
			response.set_content(fmt::format("<p>Error Status: <span style='color:red;'>{}</span></p>", response.status), "text/html");
		});

	fmt::print("Server listening on http://localhost:8080.\n");
	server.listen("localhost", 8080);

	return 0;
}
