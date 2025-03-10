/*
 * Copyright (C) Rida Bazzi, 2020
 *
 * Do not share this file with anyone
 *
 * Do not post this file or derivatives of
 * of this file online
 *
 */
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include "parser.h"


using namespace std;


void Parser::syntax_error()
{
    cout << "SYNTAX ERROR !!!!!&%!!\n";
    exit(1);
}

//extra additions
int execute_line_number = 0;
//this is for semantic error code 1, we just need to know the line number of 
//the execute section to not get anything that goes beyond that

int current_semantic_error = 0;//0 means no error

//struct for keeping track of the poly_declarations for semantic error code 1
struct PolyDeclaration {
    string name;
    int line_number;
};
vector<PolyDeclaration> poly_declarations;


//keep track of the current polynomial being parsed for semantic error code 2
struct PolynomialContext {
    string name;
    bool has_parameters;
    vector<string> parameters;
    int parameter_count;
};
PolynomialContext current_polynomial;
vector<PolynomialContext> polynomials;

//vector<string> poly_names_vector;// this vector will store all the poly_names that have been declared
vector<string> semantic_errors_vector; //this vector will store all the semantic errors that have been detected


bool tasks[5] = {false, false, false, false, false}; //gobal variable initialized to false

void Parser::store_task_num(string lexeme) {
    int n = atoi(lexeme.c_str());
    if (n > 0 && (n < 6)) {
        tasks[n-1] = true;
    }
}
// this function gets a token and checks if it is
// of the expected type. If it is, the token is
// returned, otherwise, synatx_error() is generated
// this function is particularly useful to match
// terminals in a right hand side of a rule.
// Written by Mohsen Zohrevandi
Token Parser::expect(TokenType expected_type)
{
    Token t = lexer.GetToken();
    if (t.token_type != expected_type)
        syntax_error();
    return t;
}

// Parsing
void Parser::parse_input() {
    //we are told that there is nothing after the input
    parse_program();
    expect(END_OF_FILE);
}

void Parser::parse_program() {
    //here is the CFG rule for this function
    //program -> tasks_section poly_section execute_section inputs_section
    parse_tasks_section();
    parse_poly_section();
    parse_execute_section();
    parse_inputs_section();

}

void Parser::parse_tasks_section() {
    //here is the CFG rule for this function
    //tasks_section -> TASKS num_list
    expect(TASKS);
    parse_num_list();
    //after this finishes, we should be in parse_num_list
}

void Parser::parse_num_list() {
    //here is the CFG rule for this function
    //num_list -> NUM
    //num_list -> NUM num_list
    Token t = expect(NUM);
    store_task_num(t.lexeme);
    t = lexer.peek(1);
    if (t.token_type == NUM) {
        parse_num_list();
    }
    //after this finishes, we should be at the end of the num list
    //we should go back to parse_program and it will continue with parse_poly_section
}

void Parser::parse_poly_section() {
    //here is the CFG rule for this function
    //poly_section -> POLY poly_dec_list
    Token t = expect(POLY);
    t = lexer.peek(1);
    if (t.token_type == ID) {
        parse_poly_dec_list();
    } else {
        syntax_error();
    }
    
    //after this finishes, we should be in parse_poly_dec_list
}

void Parser::parse_poly_dec_list() {
    //here is the CFG rules for this function
    //poly_dec_list -> poly_decl
    //poly_dec_list -> poly_decl poly_dec_list
    
    parse_poly_decl();//this will parse one declaration
    Token t = lexer.peek(1);
    if (t.token_type == ID) {   //ID is the start of a new poly_decl_list
        parse_poly_dec_list();

    } else if (t.token_type == EXECUTE) {   //polynomial declaration list is followed by EXECUTE
        return;
    } else {
        syntax_error();
    }
}


void Parser::parse_poly_decl() {
    //here is the CFG rule for this function
    //poly_decl -> poly_header EQUAL poly_body SEMICOLON
    parse_poly_header();
    expect(EQUAL);
    parse_poly_body();
    expect(SEMICOLON);

}

void Parser::parse_poly_header() {
    //CFG rules for this function:
    //poly_header -> poly_name
    //poly_header -> poly_name LPAREN id_list RPAREN

    //first we need to clear PolynomialContext
    current_polynomial = PolynomialContext();

    //now we can parse the poly_name
    Token t = lexer.peek(1);
    current_polynomial.name = t.lexeme;
    parse_poly_name();
    

    //now we should check if the polynomial has parameters or not
    t = lexer.peek(1);
    if (t.token_type == LPAREN) {
        current_polynomial.has_parameters = true;
        expect(LPAREN);
        parse_id_list_for_parameters();
        expect(RPAREN);
    } else {
        //means that this should nothave any parameters and the value needs to be "x"
        current_polynomial.has_parameters = false;
    }
    polynomials.push_back(current_polynomial);
}

//specific function for parsing the id_list for parameters
void Parser::parse_id_list_for_parameters() {
    //CFG rules are the same as regular id_list
    Token t = expect(ID);
    //get parameter name and add it to the vector
    current_polynomial.parameters.push_back(t.lexeme);

    //if there's a comma, then there are more parameters to parse
    t = lexer.peek(1);
    if (t.token_type == COMMA) {
        expect(COMMA);
        parse_id_list_for_parameters();
    }
}

void Parser::parse_id_list() {
    //CFG rules for this function:
    //id_list -> ID
    //id_list -> ID COMMA id_list

    expect(ID);
    Token t = lexer.peek(1);
    if (t.token_type == COMMA) {
        expect(COMMA);
        parse_id_list();
    }
}

void Parser::parse_poly_name() {
    Token t = expect(ID);

    //now we have to check if the name has been declared already
    bool found = false;
    vector<int> duplicated_line_numbers;

    //first check if the name has already been declared

    //loop through the poly_declarations vector and check if the name exists in there
    //if it isn't there, then we will add it to the vector
    for (const PolyDeclaration& declaration: poly_declarations) {
        if (declaration.name == t.lexeme) {
            if (!found) {
                found = true;
                duplicated_line_numbers.push_back(t.line_no); //add the line number to the vector
                break;
            }
        }
    }

    //now lets handle the case for if we found a duplicate
    if (found) {
        bool error_exists = false;
        //loop through the semantic_errors_vector and check if the error already exists
        for (string& error: semantic_errors_vector) {
            if (error.find("Semantic Error Code 1:") != string::npos) {
                error += " " + to_string(t.line_no);
                error_exists = true;
                break;
            }
        }
        //if the error doesn't exist, then we will create a new one
        if (!error_exists) {
            semantic_errors_vector.push_back("Semantic Error Code 1: " + to_string(t.line_no));
        }
    }
    
    poly_declarations.push_back({t.lexeme, t.line_no});
}

void Parser::parse_poly_body() {
    //CFG rules for this function:
    //poly_body -> term_list
    parse_term_list();
}

void Parser::parse_term_list() {
    //CFG rules for this function:
    //term_list -> term
    //term_list -> term add_operator term_list
    parse_term();
    Token t = lexer.peek(1);
    if (t.token_type == PLUS || t.token_type == MINUS) {
        parse_add_operator();
        parse_term_list();
    }

}

void Parser::parse_term() {
    //CFG rules for this function:
    //term -> monomial_list
    //term -> coefficient monomial_list
    //term -> coefficient
    
    Token t = lexer.peek(1);
    if (t.token_type == ID || t.token_type == LPAREN) {
        parse_monomial_list();

    } else if (t.token_type == NUM) {
        parse_coefficient();
        if (lexer.peek(1).token_type == ID || lexer.peek(1).token_type == LPAREN) {
            parse_monomial_list();
        }
    } else {
        syntax_error();
    }

}

void Parser::parse_monomial_list() {
    //CFG rules for this function:
    //monomial_list -> monomial
    //monomial_list -> monomial monomial_list
    parse_monomial();
    Token t = lexer.peek(1);
    if (t.token_type == ID || t.token_type == LPAREN) {
        parse_monomial_list();
    }
}

void Parser::parse_monomial() {
    //CFG rules for this function:
    //monomial -> primary
    //monomial -> primary exponent
    parse_primary();
    Token t = lexer.peek(1);
    if (t.token_type == POWER) {
        parse_exponent();
    }
    

}

void Parser::parse_primary() {
    //here is the CFG rules corresponding to this function
    //primary -> ID
    //primary -> LPAREN term_list RPAREN

    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        expect(ID);

        bool is_valid = false;

        if (current_polynomial.has_parameters) {
            //for polynomials with parameters, we need to check if the variable is in the parameter list
            //if it is, then we will set is_valid to true
            for (string& parameter : current_polynomial.parameters) {
                if (t.lexeme == parameter) {
                    is_valid = true;
                    break;
                }
            }
        } else {
            //for polynomials without parameters, we have to make sure the variable is "x"
            if (t.lexeme == "x") {
                is_valid = true;
            }
        }

        if (!is_valid) {
            bool error_exists = false;
            //lets check if we already have a semantic error code 2
            for (string& error : semantic_errors_vector) {
                if (error.find("Semantic Error Code 2:") != string::npos) {
                    error += " " + to_string(t.line_no);
                    error_exists = true;
                    break;
                }
            }
            //if the error doesn't exist yet, then we will create a new one
            if (!error_exists) {
                semantic_errors_vector.push_back("Semantic Error Code 2: " + to_string(t.line_no));
            }
        }
        
    } else if (t.token_type == LPAREN) {
        expect(LPAREN);
        parse_term_list();
        expect(RPAREN);
    } else {
        syntax_error();
    }

}

void Parser::parse_exponent() {
    //CFG rules for this function:
    //exponent -> POWER NUM
    Token t = lexer.peek(1);
    if (t.token_type == POWER) {
        expect(POWER);
        expect(NUM);
    } else {
        syntax_error();
    }
}

void Parser::parse_add_operator() {
    //CFG rule for this function:
    //add_operator -> PLUS
    //add_operator -> MINUS
    Token t = lexer.peek(1);
    if (t.token_type == PLUS) {
        expect(PLUS);
    } else if (t.token_type == MINUS) {
        expect(MINUS);
    } else {
        syntax_error();
    }
}

void Parser::parse_coefficient() {
    //CFG rule for this function:
    //coefficient -> NUM

    Token t = lexer.peek(1);
    if (t.token_type == NUM) {
        expect(NUM);
    } else {
        syntax_error();
    }
}


void Parser::parse_execute_section() {
    //here is the CFG rule for this function
    //execute_section -> EXECUTE statement_list
    Token t = expect(EXECUTE); 
    execute_line_number = t.line_no;
    parse_statement_list();
    
}

void Parser::parse_statement_list() {
    //here is the CFG rule for this function
    //statement_list -> statement
    //statement_list -> statement statement_list
    parse_statement();
    Token t = lexer.peek(1);
    if (t.token_type == INPUT || t.token_type == OUTPUT || t.token_type == ID) {
        parse_statement_list();
    }

}

void Parser::parse_statement() {
    //here is the CFG rule for this function
    //statement -> input_statement
    //statement -> output_statement
    //statement -> assign statement
    Token t = lexer.peek(1);
    if (t.token_type == INPUT) {
        parse_input_statement();
    } else if (t.token_type == OUTPUT) {
        parse_output_statement();
    } else if (t.token_type == ID) {
        parse_assign_statement();
    } else {
        syntax_error();
    }
}

void Parser::parse_input_statement() {
    //here is the CFG rule for this function
    //input_statement -> INPUT ID SEMICOLON
    expect(INPUT);
    expect(ID);
    expect(SEMICOLON);
    
}

void Parser::parse_output_statement() {
    //here is the CFG rule for this function
    //output_statement -> OUTPUT ID SEMICOLON
    expect(OUTPUT);
    expect(ID);
    expect(SEMICOLON);

}

void Parser::parse_assign_statement() {
    //here is the CFG rule for this function
    //assign_statement -> ID EQUAL poly_evaluation SEMICOLON
    expect(ID);
    expect(EQUAL);
    parse_poly_evaluation();
    expect(SEMICOLON);
}

void Parser::parse_poly_evaluation() {
    //here is the CFG rule for this function
    //poly_evaluation -> poly_name LPAREN argument_list RPAREN
    
    //first get the polynomial name token before parsing for semantic error code 3
    Token t = lexer.peek(1);
    string poly_name = t.lexeme;
    int line_no = t.line_no;

    const PolynomialContext* poly_info = nullptr;//to store the correct polynomial information

    //now lets check if the polynomial name is valid as in has it been declared already
    bool found = false;
    for (const PolynomialContext& p : polynomials) {
        if (p.name == poly_name) {
            poly_info = &p;
            found = true;
            break;
        }
    }

    //however it wasn't declared yet, then record the error into the semantic_errors_vector
    if (!found) {
        if (current_semantic_error == 0) {
            current_semantic_error = 3;
        }
        //cout << "DEBUG: Found undeclared polynomial: " << poly_name << " at line number: " << line_no << endl;
        bool error_exists = false;
        for (string& error: semantic_errors_vector) {
            //check if the error already exists and if it does, then just add the line number to it
            if (error.find("Semantic Error Code 3:") != string::npos) {
                //cout << "DEBUG: Before adding - error string is: " << error << endl;
                error += " " + to_string(line_no);
                //cout << "DEBUG: After adding - error string is: " << error << endl;
                error_exists = true;
                break;
            }
        }
        //if the error doesn't exist yet, then we will create a new one
        if (!error_exists) {
            semantic_errors_vector.push_back("Semantic Error Code 3: " + to_string(line_no));
        }
    } else if (tasks[3]){//this is for semantic error code 4
        //first we need to count the actual number of parameters in the argument list
        int actual_parameters = count_arguments();
        int expected_parameters = 0;

        if (poly_info->has_parameters) {
            expected_parameters = poly_info->parameters.size();
        } else {
            expected_parameters = 1;
        }

        // cout << "DEBUG: Checking " << poly_name << " at line " << line_no << ":" << endl;
        // cout << "DEBUG: Expected Parameters: " << expected_parameters << endl;
        // cout << "DEBUG: Actual params: " << actual_parameters << endl;

        //now we need to check if the actual number of parameters is not equal to the expected number of parameters
        if (actual_parameters != expected_parameters && expected_parameters != 0 && lexer.peek(1).line_no == line_no) {
            //cout << "DEBUG: MISMATCH FOUND - adding line " << line_no << endl;
            bool error_exists = false;
            current_semantic_error = 4;
            //get the expected parameter count from the poly_declarations vector
            //check if the error already exists, if it does, then just add to the line number
            for (string& error: semantic_errors_vector) {
                if (error.find("Semantic Error Code 4:") != string::npos) {
                    error += " " + to_string(line_no);
                    //cout << "DEBUG: Added line number to existing error: " << error << endl;
                    error_exists = true;
                    break;                    
                }
            }
            if (!error_exists) {
                semantic_errors_vector.push_back("Semantic Error Code 4: " + to_string(line_no));
                //cout << "DEBUG: Created new error: Semantic Error Code 4: " << line_no << endl;
            }
        } else {
           // cout << "DEBUG: No error found for " << poly_name << " at line " << line_no << endl;
        }
    } else {
        //cout << "DEBUG: Task 3 not active for" << poly_name << " at line " << line_no << endl;
    }
    
    //parse_poly_name();
    expect(ID);
    expect(LPAREN);
    parse_argument_list();
    expect(RPAREN);
}

//the point of this function is to count the number of arguments in the argument list
//this is important for semantic error code 4
int Parser::count_arguments() {
    //initialize the count to 1 because we know there is at least one argument
    int count = 0;
    //initialize the parenthesis count to 0 because we haven't seen any parentheses yet
    int parenthesis_count = 0;
    //initialize the peek_ahead to 2 because we know there is at least one argument
    int peek_ahead = 2;
    //get the first_token
    Token t = lexer.peek(peek_ahead);

    if (t.token_type == RPAREN) {
        return 0;
    }

    //first argument will be 1
    count = 0;
    //while the token is not a right parenthesis or we are still inside a parenthesis, we will 
    //keep incrementing the peek_ahead to get the parenthesis count
    while (t.token_type != END_OF_FILE) {
        if (t.token_type == LPAREN) {
            parenthesis_count++;
        } else if (t.token_type == RPAREN) {
            if (parenthesis_count == 0) {
                break;
            }
            parenthesis_count--;
        } else if (t.token_type == COMMA && parenthesis_count == 0) {
            count++;
        }
        peek_ahead++;
        t = lexer.peek(peek_ahead);
    }
    return count;
}

void Parser::parse_argument_list() {
    //here is the CFG rule for this function
    //argument_list -> argument
    //argument_list -> argument COMMA argument_list
    //expect(ID);
    parse_argument();
    Token t = lexer.peek(1);
    if (t.token_type == COMMA) {
        expect(COMMA);
        parse_argument_list();
    }
}

void Parser::parse_argument() {
    //here is the CFG rule for this function
    //argument -> ID
    //argument -> NUM
    //argument -> poly_evaluation
    Token t = lexer.peek(1);
    if (t.token_type == ID) {
        Token t2 = lexer.peek(2);
        //The problem here is that looking at F = (v, G(v)); as a POLY, the first v is ID but the G is also ID, and if there's no first v,
        //then the G is also an ID. This means it will expect the LPAREN for the G(v) but this means that we need to call parse_poly_evaluation.
        if (t2.token_type == LPAREN) {
            // if we se ID followed by LPAREN, there has to be a poly_evaluation again
            parse_poly_evaluation();
        } else {
            expect(ID);
        }
    } else if (t.token_type == NUM) {
        expect(NUM);
    } else {
        syntax_error();
    }
}

void Parser::parse_inputs_section() {
    //here is the CFG rule for this function
    //inputs_section -> INPUTS num_list
    expect(INPUTS);
    parse_num_list();
}

void Parser::check_semantic_error_code1() {
    //go through and print all the collected semantic error code 1s
    if (!semantic_errors_vector.empty()) {
        for (string& error: semantic_errors_vector) {
            //only check the error if it contains "Semantic Error Code 1:"
            if (error.find("Semantic Error Code 1:") != string::npos) {
                //extract all numbers from the error string and put them into a vector
                vector<int> numbers;
                size_t pos = error.find(":") + 1; //pos is for position of the number
                string number_str;
                //loop through the error string and extract all the numbers
                while (pos < error.length()) {
                if (isdigit(error[pos])) {
                    number_str += error[pos];
                } else if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                    number_str = "";
                    }
                    pos++;
                }
                if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                }

                //we have to filter out the numbers that are greater than the execute_line_number
                vector<int> filtered_numbers;
                //cout << "The execute line number is: " << execute_line_number << endl;
                for (int num: numbers) {
                    if (num < execute_line_number) {
                        filtered_numbers.push_back(num);
                    }
                }

                //now we have all the numbers in the vector
                //now we need to print them in ascending order
                if (!filtered_numbers.empty()) {
                sort(filtered_numbers.begin(), filtered_numbers.end());

                //reconstruct the error message now that they are in ascending order
                string sorted_error = "Semantic Error Code 1:";
                for (int num: filtered_numbers) {
                    sorted_error += " " + to_string(num);
                }
                    cout << sorted_error << endl;
                }
            }
        }
    }
}

void Parser::check_semantic_error_code2() {
    //go through and print all the collected semantic error code 1s
    if (!semantic_errors_vector.empty()) {
        for (string& error: semantic_errors_vector) {
            //only check the error if it contains "Semantic Error Code 2:"
            if (error.find("Semantic Error Code 2:") != string::npos) {
                //extract all numbers from the error string and put them into a vector
                vector<int> numbers;
                size_t pos = error.find(":") + 1; //pos is for position of the number
                string number_str;
                //loop through the error string and extract all the numbers
                while (pos < error.length()) {
                if (isdigit(error[pos])) {
                    number_str += error[pos];
                } else if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                        number_str = "";
                    }
                    pos++;
                }
                if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                }

                //we have to filter out the numbers that are greater than the execute_line_number
                vector<int> filtered_numbers;
                //cout << "The execute line number is: " << execute_line_number << endl;
                for (int num: numbers) {
                    if (num < execute_line_number) {
                        filtered_numbers.push_back(num);
                    }
                }

                //now we have all the numbers in the vector
                //now we need to print them in ascending order
                if (!filtered_numbers.empty()) {
                    sort(filtered_numbers.begin(), filtered_numbers.end());

                //reconstruct the error message now that they are in ascending order
                string sorted_error = "Semantic Error Code 2:";
                for (int num: filtered_numbers) {
                    sorted_error += " " + to_string(num);
                }
                    cout << sorted_error << endl;
                }
            }
        }
    }
}

void Parser::check_semantic_error_code3() {
    //go through and print all the collected semantic error code 3s
    //we can just copy paste the code from previous semantic error functions and change the wording
    //and error message because the logic is the same, we are just checking for whether an error code exists
    //and if it does, then we will make sure the line numbers are in ascending order and then print the error
    if (!semantic_errors_vector.empty()) {
        for (string& error: semantic_errors_vector) {
            //only check the error if it contains "Semantic Error Code 3:"
            if (error.find("Semantic Error Code 3:") != string::npos) {
                //extract all numbers from the error string and put them into a vector
                vector<int> numbers;
                size_t pos = error.find(":") + 1; //pos is for position of the number
                string number_str;
                //loop through the error string and extract all the numbers
                while (pos < error.length()) {
                if (isdigit(error[pos])) {
                    number_str += error[pos];
                } else if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                        number_str = "";
                    }
                    pos++;
                }
                if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                }

                //we have to filter out the numbers that are less than the execute_line_number
                //opposite of what we did in the other two functions because this error message is for after the execute section
                // vector<int> filtered_numbers;
                // //cout << "The execute line number is: " << execute_line_number << endl;
                // for (int num: numbers) {
                //     if (num >= execute_line_number) {
                //         filtered_numbers.push_back(num);
                //     }
                // }

                //now we have all the numbers in the vector
                //now we need to print them in ascending order
                if (!numbers.empty()) {
                    sort(numbers.begin(), numbers.end());

                //reconstruct the error message now that they are in ascending order
                string sorted_error = "Semantic Error Code 3:";
                for (int num: numbers) {
                    sorted_error += " " + to_string(num);
                }
                    cout << sorted_error << endl;
                }
            }
        }
    }
}

void Parser::check_semantic_error_code4() {
    //go through and print all the collected semantic error code 4s
    //we can just copy paste the code from previous semantic error functions and change the wording
    //and error message because the logic is the same, we are just checking for whether an error code exists
    //and if it does, then we will make sure the line numbers are in ascending order and then print the error
    if (!semantic_errors_vector.empty()) {
        for (string& error: semantic_errors_vector) {
            //only check the error if it contains "Semantic Error Code 4:"
            if (error.find("Semantic Error Code 4:") != string::npos) {
                //extract all numbers from the error string and put them into a vector
                vector<int> numbers;
                size_t pos = error.find(":") + 1; //pos is for position of the number
                string number_str;
                //loop through the error string and extract all the numbers
                while (pos < error.length()) {
                if (isdigit(error[pos])) {
                    number_str += error[pos];
                } else if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                        number_str = "";
                    }
                    pos++;
                }
                if (!number_str.empty()) {
                    numbers.push_back(stoi(number_str));
                }

                //now we have all the numbers in the vector
                //now we need to print them in ascending order
                if (!numbers.empty()) {
                    sort(numbers.begin(), numbers.end());

                //reconstruct the error message now that they are in ascending order
                string sorted_error = "Semantic Error Code 4:";
                for (int num: numbers) {
                    sorted_error += " " + to_string(num);
                }
                    cout << sorted_error << endl;
                }
            }
        }
    }
}


// This function is simply to illustrate the GetToken() function
// you will not need it for your project and you can delete it
// the function also illustrates the use of peek()
void Parser::ConsumeAllInput()
{
    Token token;
    int i = 1;

    token = lexer.peek(i);
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        i = i + 1;
        token = lexer.peek(i);
        token.Print();
    }

    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}

int main()
{
    // note: the parser class has a lexer object instantiated in it. You should not be declaring
    // a separate lexer object. You can access the lexer object in the parser functions as shown in the
    // example method Parser::ConsumeAllInput
    // If you declare another lexer object, lexical analysis will not work correctly
    Parser parser;

    //parser.ConsumeAllInput();
    parser.parse_input();
    parser.check_semantic_error_code1();
    parser.check_semantic_error_code2();
    parser.check_semantic_error_code3();
    parser.check_semantic_error_code4();
}
