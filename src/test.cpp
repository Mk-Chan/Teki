#include <iostream>

#include "move.h"
#include "neural/loader.h"
#include "neural/factory.h"
#include "neural/encoder.h"
#include "neural/optionsdict.h"
#include "neural/move_index.h"

int main()
{
    std::ios_base::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);

    auto network_factory = lczero::NetworkFactory::Get();
    auto weights =  lczero::LoadWeightsFromFile("weights_479.txt.gz");
    auto network = network_factory->Create("cudnn", weights, lczero::OptionsDict {});
    auto comp = network->NewComputation();
    std::string line;
    std::cout << "Enter FEN\n";
    std::getline(std::cin, line);
    std::stringstream ss {line};

    Position pos;
    pos.init(ss);

    std::vector<Move> mlist;
    pos.generate_legal_movelist(mlist);

    for (Move& m : mlist) {
        auto pos_hist = PositionHistory {};

        Position p {pos};
        pos_hist.Reset(p);

        pos_hist.Append(m);
        p = pos_hist.Last();

        auto plane = lczero::EncodePositionForNN(pos_hist, pos_hist.GetLength());
        lczero::InputPlanes planes {plane};
        comp->AddInput(std::move(planes));
    }

    comp->ComputeBlocking();

    for (Move& m : mlist) {
        Position p {pos};
        p.make_move(m);
        p.display();
        auto st = get_move_string(m, false);
        int i = -1;
        for (i = 0; i < 1858; ++i)
            if (st == kIdxToMove[i])
                break;
        std::cout << "i: " << i << ", move: " << kIdxToMove[i] << std::endl;
        float Q = -comp->GetQVal(0);
        float P = comp->GetPVal(0, i);
        std::cout << "Q: " << Q << ", P: " << P << std::endl;
    }
}
