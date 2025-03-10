/*
 * Copyright (C) Rida Bazzi, 2019
 *
 * Do not share this file with anyone
 */
#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include "lexer.h"

// struct poly_header_t {
//   Token name;        // holds the name of the polynomial
//   vector<Token> id_list; // list of parameters aka IDs for the polynomial
// };

class Parser {
  public:
  
    void parse_input();
    void parse_program();
    void parse_poly_decl();
    void parse_tasks_section();
    void parse_poly_section();
    void parse_execute_section();
    void parse_inputs_section();
    void parse_poly_header();
    void parse_poly_body();
    void parse_primary();
    void parse_term_list();
    void parse_poly_dec_list();
    void parse_poly_name();
    void parse_num_list();
    void parse_statement_list();
    void parse_statement();
    void parse_id_list();
    void parse_term();
    void parse_monomial_list();
    void parse_monomial();
    void parse_coefficient();
    void parse_exponent();
    void parse_add_operator();
    void parse_input_statement();
    void parse_output_statement();
    void parse_assign_statement();
    void parse_poly_evaluation();
    void parse_argument_list();
    void parse_argument();
    void store_task_num(std::string lexeme);
    void check_semantic_error_code1();
    void parse_id_list_for_parameters();
    void check_semantic_error_code2();
    void check_semantic_error_code3();
    int count_arguments();
    void check_semantic_error_code4();
    
    void ConsumeAllInput();

  private:
    LexicalAnalyzer lexer;
    void syntax_error();
    Token expect(TokenType expected_type);
};

#endif

