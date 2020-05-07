
#include "simple_json.h"

#ifndef DEPENDING
#include "test1.pb.h"
#include "test2.pb.h"
#include "test1_converter.h"
#include "test2_converter.h"
#endif

#include <sstream>

int
main()
{
    pkg::test1::Msg1_m    m;
    pkg::test1::Msg2_m_Msg2Sub_m * m2sub = NULL;

    m.set_type(pkg::test1::VALUE2);
    m.add_stuff(true);
    m.add_stuff(false);
    m.add_astr("one");
    m.add_astr("two");
    m.mutable_msg3()->add_values(8);
    m.mutable_msg3()->add_values(9);
    m.mutable_msg3()->add_values(10);
    m.set_unsignedint(4);
    m.mutable_msg2()->set_type(pkg::test1::VALUE4);
    m.mutable_msg2()->set_signedint(-4);
    m.mutable_msg2()->set_unsigned64(8);
    m.mutable_msg2()->add_msg1s()->set_type(pkg::test1::VALUE1);
    m.mutable_msg2()->add_msg1s()->set_type(pkg::test1::VALUE2);
    m.mutable_msg2()->add_ones(pkg::test1::VALUE1);
    m.mutable_msg2()->add_ones(pkg::test1::VALUE2);
    m.mutable_msg2()->add_manyunsigneds(9);
    m.mutable_msg2()->add_manyunsigneds(10);
    m.mutable_msg2()->add_manyunsigneds(11);
    m2sub = m.mutable_msg2()->add_twosubs();
    m2sub->add_subvalues(14);
    m2sub->add_subvalues(18);
    m2sub = m.mutable_msg2()->add_twosubs();
    m2sub->add_subvalues(19);
    m2sub->add_subvalues(26);
    m.set_en3(pkg::test2::VALUE6);
    m.set_astr2("stuff");
    m.set_thingy(false);

    std::cout << "made a Msg1_m:\n"
              << m.DebugString()
              << "\n";

    SimpleJson::Property * p =
        pkg::test1::JsonProtoConvert_Msg1_m(m);
    if (p == NULL)
    {
        std::cout << "failure making json\n";
        return 1;
    }

    std::ostringstream  str;

    str << p;
    delete p;

    SimpleJson::ObjectProperty * o = NULL;

    std::cout << "made a json:\n"
              << str.str()
              << "\n\n";

    p = SimpleJson::parseJson(str.str());
    if (p == NULL)
    {
        std::cout << "failure parsing my own json output\n";
        return 1;
    }

    o = p->cast<SimpleJson::ObjectProperty>();
    if (o == NULL)
    {
        std::cout << "parsing my own json output was not an Object\n";
        return 1;
    }

    pkg::test1::Msg1_m    n;

    if (pkg::test1::JsonProtoConvert_Msg1_m(n, o) == false)
    {
        std::cout << "failure converting back to protobuf\n";
        return 1;
    }
    else
    {
        std::cout << "made a Msg1_m:\n"
                  << n.DebugString();
    }

    delete p;

    return 0;
}

