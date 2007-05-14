/*
 * Copyright (c) 2003-2007, John Wiegley.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of New Artisans LLC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "session.h"

namespace ledger {

session_t * session_t::current = NULL;

#if 0
boost::mutex session_t::session_mutex;
#endif

static void initialize();
static void shutdown();

void set_session_context(session_t * session)
{
#if 0
  session_t::session_mutex.lock();
#endif

  if (session && ! session_t::current) {
    initialize();
  }
  else if (! session && session_t::current) {
    shutdown();
#if 0
    session_t::session_mutex.unlock();
#endif
  }

  session_t::current = session;
}

void release_session_context()
{
#if 0
  session_t::session_mutex.unlock();
#endif
}

void session_t::read_journal(std::istream&   in,
			     const path&     pathname,
			     xml::builder_t& builder)
{
#if 0
  if (! master)
    master = journal->master;
#endif

  for (ptr_list<parser_t>::iterator i = parsers.begin();
       i != parsers.end();
       i++)
    if (i->test(in))
      i->parse(in, pathname, builder);
}

void session_t::read_journal(const path&     pathname,
			     xml::builder_t& builder)
{
#if 0
  journal->sources.push_back(pathname);
#endif

  if (! exists(pathname))
    throw_(std::logic_error, "Cannot read file" << pathname);

  ifstream stream(pathname);
  read_journal(stream, pathname, builder);
}

void session_t::read_init()
{
  if (! init_file)
    return;

  if (! exists(*init_file))
    throw_(std::logic_error, "Cannot read init file" << *init_file);

  ifstream init(*init_file);

  // jww (2006-09-15): Read initialization options here!
}

journal_t * session_t::read_data(const string& master_account)
{
#if 1
  return NULL;
#else
  if (data_file.empty())
    throw_(parse_error, "No journal file was specified (please use -f)");

  TRACE_START(parser, 1, "Parsing journal file");

  journal_t * journal = new_journal();
  journal->document = new xml::document_t;
  journal->document->set_top(xml::wrap_node(journal->document, journal));

  unsigned int entry_count = 0;

  DEBUG("ledger.cache", "3. use_cache = " << use_cache);

  if (use_cache && cache_file) {
    DEBUG("ledger.cache", "using_cache " << cache_file->string());
    cache_dirty = true;
    if (exists(*cache_file)) {
      scoped_variable<optional<path> >
	save_price_db(journal->price_db, price_db);

      ifstream stream(*cache_file);
      entry_count += read_journal(stream, journal, NULL, data_file);
      if (entry_count > 0)
	cache_dirty = false;
    }
  }

  if (entry_count == 0) {
    account_t * acct = NULL;
    if (! master_account.empty())
      acct = journal->find_account(master_account);

    journal->price_db = price_db;
    if (journal->price_db && exists(*journal->price_db)) {
      if (read_journal(*journal->price_db, journal)) {
	throw_(parse_error, "Entries not allowed in price history file");
      } else {
	DEBUG("ledger.cache",
	      "read price database " << journal->price_db->string());
	journal->sources.pop_back();
      }
    }

    DEBUG("ledger.cache", "rejected cache, parsing " << data_file.string());
    if (data_file == "-") {
      use_cache = false;
      journal->sources.push_back("<stdin>");
      entry_count += read_journal(std::cin, journal, acct);
    }
    else if (exists(data_file)) {
      entry_count += read_journal(data_file, journal, acct);
      if (journal->price_db)
	journal->sources.push_back(*journal->price_db);
    }
  }

  VERIFY(journal->valid());

  if (entry_count == 0)
    throw_(parse_error, "Failed to locate any journal entries; "
	   "did you specify a valid file with -f?");

  TRACE_STOP(parser, 1);

  return journal;
#endif
}

bool session_t::resolve(const string& name, value_t& result,
			xml::xpath_t::scope_t * locals)
{
  const char * p = name.c_str();
  switch (*p) {
  case 'd':
    if (name == "date_format") {
      // jww (2007-04-18): What to do here?
#if 0
      result.set_string(moment_t::output_format);
#endif
      return true;
    }
    break;

  case 'n':
    switch (*++p) {
    case 'o':
      if (name == "now") {
	result = now;
	return true;
      }
      break;
    }
    break;

  case 'r':
    if (name == "register_format") {
      result = register_format;
      return true;
    }
    break;
  }

  return xml::xpath_t::scope_t::resolve(name, result, locals);
}

xml::xpath_t::op_t * session_t::lookup(const string& name)
{
  const char * p = name.c_str();
  switch (*p) {
  case 'o':
    if (std::strncmp(p, "option_", 7) == 0) {
      p = p + 7;
      switch (*p) {
      case 'd':
	if (std::strcmp(p, "debug") == 0)
	  return MAKE_FUNCTOR(session_t, option_debug);
	break;

      case 'f':
	if (! *(p + 1) || std::strcmp(p, "file") == 0)
	  return MAKE_FUNCTOR(session_t, option_file);
	break;

      case 't':
	if (std::strcmp(p, "trace") == 0)
	  return MAKE_FUNCTOR(session_t, option_trace);
	break;

      case 'v':
	if (! *(p + 1) || std::strcmp(p, "verbose") == 0)
	  return MAKE_FUNCTOR(session_t, option_verbose);
	else if (std::strcmp(p, "verify") == 0)
	  return MAKE_FUNCTOR(session_t, option_verify);
	break;
      }
    }
    break;
  }

  return xml::xpath_t::scope_t::lookup(name);
}

// jww (2007-04-26): All of Ledger should be accessed through a
// session_t object
static void initialize()
{
  amount_t::initialize();
  xml::xpath_t::initialize();
}

static void shutdown()
{
  xml::xpath_t::shutdown();
  amount_t::shutdown();
}

} // namespace ledger
