function sortword(w)
    String(sort(Vector{Char}(w)))
end

function countword(a,ws,w)
    a[ws]=push!(get(a, ws, []), w)
end

function main()
    filename = joinpath(homedir(),"Downloads","enable1.txt");
    a = Dict()
    open(filename) do f
        for w in eachline(f)
            ws = sortword(w)
            countword(a,ws,w)
        end
    end
    A=sort(collect(values(a)),by=length,rev=true)
    view(A,1:10)
end

start = time()
main()
endtm = time()
elaps = endtm - start;
print(elaps);


