#include <Parsers/ASTIdentifier.h>
#include <Parsers/ASTDropQuery.h>

#include <Parsers/CommonParsers.h>
#include <Parsers/ParserDropQuery.h>

#include <Common/typeid_cast.h>


namespace DB
{

namespace ErrorCodes
{
    extern const int SYNTAX_ERROR;
}

bool ParserDropQuery::parseImpl(Pos & pos, ASTPtr & node, Expected & expected)
{
    ParserKeyword s_drop("DROP");
    ParserKeyword s_detach("DETACH");
    ParserKeyword s_truncate("TRUNCATE");

    if (s_drop.ignore(pos, expected))
        return parseDropQuery(pos, node, expected);
    else if (s_detach.ignore(pos, expected))
        return parseDetachQuery(pos, node, expected);
    else if (s_truncate.ignore(pos, expected))
        return parseTruncateQuery(pos, node, expected);
    else
        return false;
}

bool ParserDropQuery::parseDetachQuery(Pos & pos, ASTPtr & node, Expected & expected)
{
    if (parseDropQuery(pos, node, expected))
    {
        ASTDropQuery * drop_query = static_cast<ASTDropQuery*>(node.get());
        drop_query->kind = ASTDropQuery::Kind::Detach;
        return true;
    }
    return false;
}

bool ParserDropQuery::parseTruncateQuery(Pos & pos, ASTPtr & node, Expected & expected)
{
    if (parseDropQuery(pos, node, expected))
    {
        ASTDropQuery * drop_query = static_cast<ASTDropQuery*>(node.get());
        drop_query->kind = ASTDropQuery::Kind::Truncate;
        return true;
    }
    return false;
}

bool ParserDropQuery::parseDropQuery(Pos & pos, ASTPtr & node, Expected & expected)
{
    ParserKeyword s_temporary("TEMPORARY");
    ParserKeyword s_table("TABLE");
    ParserKeyword s_database("DATABASE");
    ParserKeyword s_dictionary("DICTIONARY");
    ParserToken s_dot(TokenType::Dot);
    ParserKeyword s_if_exists("IF EXISTS");
    ParserIdentifier name_p;

    ASTPtr database;
    ASTPtr table;
    ASTPtr dictionary;
    String cluster_str;
    bool if_exists = false;
    bool temporary = false;

    if (s_database.ignore(pos, expected))
    {
        if (s_if_exists.ignore(pos, expected))
            if_exists = true;

        if (!name_p.parse(pos, database, expected))
            return false;

        if (ParserKeyword{"ON"}.ignore(pos, expected))
        {
            if (!ASTQueryWithOnCluster::parse(pos, cluster_str, expected))
                return false;
        }
    }
    else
    {
        if (s_temporary.ignore(pos, expected))
            temporary = true;

        ASTPtr * argument = nullptr;
        if (s_table.ignore(pos, expected))
            argument = &table;
        else if (s_dictionary.ignore(pos, expected))
            argument = &dictionary;
        else
            return false;

        if (s_if_exists.ignore(pos, expected))
            if_exists = true;

        if (!name_p.parse(pos, *argument, expected))
            return false;

        if (s_dot.ignore(pos, expected))
        {
            database = *argument;
            if (!name_p.parse(pos, *argument, expected))
                return false;
        }

        if (ParserKeyword{"ON"}.ignore(pos, expected))
        {
            if (!ASTQueryWithOnCluster::parse(pos, cluster_str, expected))
                return false;
        }
    }

    auto query = std::make_shared<ASTDropQuery>();
    node = query;

    query->kind = ASTDropQuery::Kind::Drop;
    query->if_exists = if_exists;
    query->temporary = temporary;
    if (database)
        query->database = typeid_cast<ASTIdentifier &>(*database).name;
    if (table)
        query->table = typeid_cast<ASTIdentifier &>(*table).name;
    if (dictionary)
        query->dictionary = typeid_cast<ASTIdentifier &>(*dictionary).name;
    query->cluster = cluster_str;

    return true;
}

}
