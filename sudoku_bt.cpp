#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <boost/exception/all.hpp>
#include <boost/multi_array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/vector_tie.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#define PRINT(X) do{ std::cout << #X " => " << (X) << "\n";}while(0)

namespace sudoku{
        
        template<class Attr_t>
        struct basic_board
        {
                using attr_t = Attr_t;
                using board_t = boost::multi_array<attr_t,2>;
        private:
                static constexpr size_t width_{9};
                static constexpr size_t height_{9};
        public:

                basic_board():
                        board_(boost::extents[width_][height_])
                {}
                basic_board(const basic_board&)=default;
                basic_board(basic_board&&)=default;
                basic_board& operator=(const basic_board&)=default;
                basic_board& operator=(basic_board&&)=default;

                attr_t& operator()(size_t x, size_t y){ return board_[x][y]; }
                attr_t const& operator()(size_t x, size_t y)const{ return board_[x][y]; }
                attr_t& get(size_t x, size_t y){ return board_[x][y]; }
                attr_t const& get(size_t x, size_t y)const{ return board_[x][y]; }
                void fill(attr_t const& proto){
                        for(size_t i=0;i!=width();++i){
                                for(size_t j=0;j!=height();++j){
                                        get(i,j) = proto;
                                }
                        }
                }

                size_t width()const{return width_;}
                size_t height()const{return height_;}
                
        private:
                board_t board_;
        };

        using board_t = basic_board<std::uint8_t>;

        // view to add an attribute to a board
        template<class Board_Type, class Attr_Type>
        struct board_attr_t{
                using board_t = Board_Type;
                using attr_t = Attr_Type;
                using attr_array_t = boost::multi_array<attr_t, 2 >;

                explicit board_attr_t(board_t const& impl)
                        :impl_(impl)
                         , attr_(boost::extents[impl_.width()][impl_.height()])
                {}
                board_attr_t(const board_attr_t&)=default;

                attr_t& attr_get(size_t i, size_t j){ return attr_[i][j]; }
                attr_t const& attr_get(size_t i, size_t j)const{ return attr_[i][j]; }
                void attr_fill(const attr_t& proto){
                        for(size_t i=0;i!=impl_.width();++i)
                                for(size_t j=0;j!=impl_.height();++j)
                                        attr_get(i,j) = proto;
                }

                decltype(auto) operator()(size_t x, size_t y){ return impl_.operator()(x,y); }
                decltype(auto) operator()(size_t x, size_t y)const{ return impl_.operator()(x,y); }
                decltype(auto) get(size_t x, size_t y){ return impl_.get(x,y);}
                decltype(auto) get(size_t x, size_t y)const{ return impl_.get(x,y);}
                decltype(auto) width()const{return impl_.width();}
                decltype(auto) height()const{return impl_.height();}
        private:
                Board_Type impl_;
                attr_array_t attr_;
        };


        // takes a string, removed all "|-", then all whitespace considers empty
        void parse_input(board_t& self, std::string input){
                boost::erase_all(input,"|");
                boost::erase_all(input,"-");
                boost::erase_all(input,"\n");
                if( input.size() != 9*9 )
                        BOOST_THROW_EXCEPTION(std::domain_error("not a valid string"));
                for( char c : input )
                        if( ! ( std::isdigit(c) || std::isspace(c)) )
                                BOOST_THROW_EXCEPTION(std::domain_error("not a valid string"));
                auto iter = input.cbegin();
                for(size_t y=self.height();y;){
                        --y;
                        for(size_t x=0;x!=self.width();++x,++iter){
                                self(x,y) = std::isspace(*iter) ? 0 : boost::lexical_cast<int>(*iter); 
                        }
                }
        }

        // for checking if solution is valid
                
        // return false iff there exits 2 of the same non-empty numbers in the row/column
        template<class Board_Type>
        auto test_generic(Board_Type&& board, size_t i, bool test_x){
                std::set<int> s;
                for(size_t j=0;j!=9;++j){
                        int val = test_x ? board(i,j) : board(j,i);
                        if( val == 0 )
                                continue;
                        if( s.count( val ) == 1 )
                               return false; 
                        s.insert(val);
                }
                assert( s.size() <= 9 );
                return true;
        };

        template<class Board_Type>
        auto test_square(Board_Type&& board, size_t x, size_t y){
                std::set<int> s;
                size_t xo = ( x / 3 ) * 3;
                size_t yo = ( y / 3 ) * 3;
                for(size_t i=0;i!=3;++i){
                        for(size_t j=0;j!=3;++j){
                                auto val = board(xo+i,yo+j);
                                if( val == 0 )
                                        continue;
                                if( s.count( val )  == 1 )
                                       return false; 
                                s.insert(val);
                        }
                }
                assert( s.size() <= 9 );
                return true;
        };
        
        template<class Board_Type>
        auto test_solution(Board_Type&& board){
                for(size_t i=0;i!=9;++i){
                        if( ! ( test_generic(board,i,false) && test_generic(board,i,false) && test_square( board, i % 3, i / 3 ) ) )
                                return false;
                }
                return true;
        }


        namespace io{
                struct pretty_printer{

                        explicit pretty_printer(std::ostream& ostr):ostr_(&ostr){}
                        template<class Access>
                        void operator()(Access&& access)const
                        {
                                *ostr_ << std::string(9+4,'-') << "\n";
                                for(size_t y=9;y!=0;){

                                        --y;
                                        for(size_t x=0;x!=9;++x){
                                                if( x % 3 == 0 )
                                                        *ostr_ << "|";
                                                *ostr_ << access(x,y);
                                        }
                                        *ostr_ << "|\n";
                                        if( y % 3 == 0 )  
                                                *ostr_ << std::string(9+4,'-') << "\n";
                                }
                        }
                private:
                        std::ostream* ostr_;
                };


                #if 0
                struct color_formatter{
                        static std::string color_red(const std::string& s){
                                return "\033[0;36m" + s + "\033[0;m";
                        }
                        static std::string color_purple(const std::string& s){
                                return "\033[0;35m" + s + "\033[0;m";
                        }
                        static std::string color_blue(const std::string& s){
                                return "\033[0;34m" + s + "\033[0;m";
                        }
                };
                #else
                struct color_formatter{
                        static std::string color_red(const std::string& s){
                                return s;
                        }
                        static std::string color_purple(const std::string& s){
                                return s;
                        }
                        static std::string color_blue(const std::string& s){
                                return s;
                        }
                };
                #endif

        }
}

#include <random>

template<class Board_Type>
struct monte_carlo_solver{
        using board_t = Board_Type;
        using attr_board_t = sudoku::board_attr_t<board_t,std::uint8_t>;

        std::vector<attr_board_t> solve(board_t const&  puzzle)const{
                attr_board_t proto(puzzle);
                proto.attr_fill(0);
                
                std::random_device rd;
                std::default_random_engine en(rd());
                std::uniform_int_distribution<int> dist(1,9);
                auto r = [&dist,&en](){ return dist(en); };

                for(;;){
                        attr_board_t generate_sol( proto );
                        for(size_t x=0;x!=9;++x){
                                for(size_t y=0;y!=9;++y){
                                        if( generate_sol(x,y) == 0 ){
                                                generate_sol(x,y) = r();
                                                generate_sol.attr_get(x,y) = 1;
                                        }
                                }
                        }

                        if( test_solution( generate_sol ) ){
                                return std::vector<attr_board_t>{ generate_sol };
                        }
                }
        }
};
template<class Board_Type>
struct bruteforce_solver{
        using board_t = Board_Type;
        using attr_board_t = sudoku::board_attr_t<board_t,std::uint8_t>;
        /*
         * for each space to be filled
         *      find possible value for space
         *
         *
         */
        std::vector<attr_board_t> solve(board_t const&  puzzle)const{
                // check valid atm
                if( ! test_solution( puzzle) ){
                        BOOST_THROW_EXCEPTION(std::domain_error("invalid puzzle")); 
                }
                attr_board_t attr_puzzle (puzzle);
                attr_puzzle.attr_fill(0);
                std::vector<attr_board_t> sol;

                // explicit stack
                std::vector<attr_board_t> stack;
                stack.emplace_back(puzzle);

                // white there is a puzzle on the stack
                //      remember to puzzle on the top and the stack and pop it
                //      if the puzzle doesn't have any empty square
                //              it's a solution so push it to solution stack
                //      push all potentially valid solutions for the square on the stack
                //
                for(;stack.size();){

                        attr_board_t subject( stack.back());
                        stack.pop_back();

                        // find first non-empty
                        size_t x, y;
                        boost::fusion::vector_tie(x,y) = [&]()->boost::fusion::vector<size_t,size_t>{
                                for(size_t x=0;x!=subject.width();++x){
                                        for(size_t y=0;y!=subject.height();++y){
                                                if( subject(x,y) == 0 ){
                                                        return boost::fusion::make_vector(x,y);
                                                }
                                        }
                                }
                                return boost::fusion::make_vector(-1,-1);
                        }();

                        if( x == -1 ){
                                assert( y == -1 );
                                assert( test_solution( subject ));
                                sol.push_back(subject);
                                continue;
                        }


                        for(size_t i=1;i!=10;++i){
                                attr_board_t b(subject);
                                b(x,y) = i;
                                b.attr_get(x,y) = 1;
                                // is it valid
                                if( test_generic(b,x,true) && test_generic(b,y,false) && test_square(b,x,y) )
                                        stack.push_back(b);
                        }
                }

                return sol;
        }
};

struct driver{
        driver(int argc, char* argv[]){
        }
        int run(){

                sudoku::board_t puzzle;

                const char* input{
R"(
-------------
|53 | 7 |   |
|6  |195|   |
| 98|   | 6 |
-------------
|8  | 6 |  3|
|4  |8 3|  1|
|7  | 2 |  6|
-------------
| 6 |   |28 |
|   |419|  5|
|   | 8 | 79|
-------------
)"};
                
                parse_input(puzzle, input);

                std::cout << input << "\n";
                        

                auto print = [](auto&& board){
                        (sudoku::io::pretty_printer((std::cout)))(
                                [&board](auto x, auto y)->std::string{
                                        auto const val = static_cast<int>(board(x,y));
                                        if( val == 0 )
                                                return " ";
                                        switch(board.attr_get(x,y)){
                                                case 0:
                                                        return sudoku::io::color_formatter::
                                                                color_red(boost::lexical_cast<std::string>(val));
                                                case 1:
                                                        return sudoku::io::color_formatter::
                                                                color_purple(boost::lexical_cast<std::string>(val));
                                                default:
                                                        assert( 0 );
                                        }
                                }
                        );
                };

                std::cout << "solving...\n";

                bruteforce_solver<sudoku::board_t> bf_solver;
                auto bf_sol = bf_solver.solve( puzzle );
                for( auto const& result : bf_sol ){
                        print(result);
                } 
                #if 0
                monte_carlo_solver<sudoku::board_t> mc_solver;
                auto mc_sol = mc_solver.solve( puzzle );
                for( auto const& result : mc_sol ){
                        print(result);
                } 
                #endif
                return EXIT_SUCCESS;
        }
};

int main(int argc, char* argv[]){
        try{
                driver drv(argc,argv);
                return drv.run();
        } catch( const std::exception& e){
                std::cerr 
                        << "Caught exception: "
                        << boost::diagnostic_information(e)
                        ;
                return EXIT_FAILURE;
        }
}
