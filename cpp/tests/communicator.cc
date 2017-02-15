#include <iostream>
#include <numeric>
#include "catch.hpp"

#include "sopt/config.h"
#include "sopt/mpi/communicator.h"

using namespace sopt;

#ifdef SOPT_MPI
TEST_CASE("Creates an mpi communicator") {
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  auto const world = mpi::Communicator::World();

  SECTION("General stuff") {
    REQUIRE(*world == MPI_COMM_WORLD);
    REQUIRE(static_cast<t_int>(world.rank()) == rank);
    REQUIRE(static_cast<t_int>(world.size()) == size);

    mpi::Communicator shallow = world;
    CHECK(*shallow == *world);
  }

  SECTION("Duplicate") {
    mpi::Communicator dup = world.duplicate();
    CHECK(*dup != *world);
  }

  SECTION("Scatter") {
    if(world.rank() == world.root_id()) {
      std::vector<t_int> scattered(world.size());
      std::iota(scattered.begin(), scattered.end(), 2);
      auto const result = world.scatter_one(scattered);
      CHECK(result == world.rank() + 2);
    } else {
      auto const result = world.scatter_one<t_int>();
      CHECK(result == world.rank() + 2);
    }
  }

  SECTION("ScatterV") {
    std::vector<t_int> sizes(world.size()), displs(world.size());
    for(t_uint i(0); i < world.rank(); ++i)
      sizes[i] = world.rank() * 2 + i;
    for(t_uint i(1); i < world.rank(); ++i)
      displs[i] = displs[i - 1] + sizes[i - 1];
    Vector<t_int> const sendee
        = Vector<t_int>::Random(std::accumulate(sizes.begin(), sizes.end(), 0));
    auto const result = world.rank() == world.root_id() ?
                            world.scatterv(sendee, sizes) :
                            world.scatterv<t_int>(sizes[world.rank()]);
    CHECK(result.isApprox(sendee.segment(displs[world.rank()], sizes[world.rank()])));
  }

  SECTION("All sum all over image") {
    Image<t_int> image(2, 2);
    image.fill(world.rank());
    world.all_sum_all(image);
    CHECK((2 * image == world.size() * (world.size() - 1)).all());
  }

  SECTION("Broadcast") {
    auto const result = world.broadcast(world.root_id() == world.rank() ? 5 : 2, world.root_id());
    CHECK(result == 5);

    Vector<t_int> y0(3);
    y0 << 3, 2, 1;
    auto const y
        = world.rank() == world.root_id() ? world.broadcast(y0) : world.broadcast<Vector<t_int>>();
    CHECK(y == y0);

    std::vector<t_int> v0 = {3, 2, 1};
    auto const v = world.rank() == world.root_id() ? world.broadcast(v0) :
                                                     world.broadcast<std::vector<t_int>>();
    CHECK(v[0] == v0[0]);
    CHECK(v[1] == v0[1]);
    CHECK(v[2] == v0[2]);

    Image<t_int> image0(2, 2);
    image0 << 3, 2, 1, 0;
    auto const image = world.rank() == world.root_id() ? world.broadcast(image0) :
                                                         world.broadcast<Image<t_int>>();
    CHECK(image.matrix() == image0.matrix());
  }
}
#endif
