module Brplot

using Brplot_jll

plot(y::Real)                    = @ccall Brplot_jll.libbrplot.brp_1(y::Cdouble, 0::Cint)::Cvoid
plot(y::Real, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_1(y::Cdouble, group_id::Cint)::Cvoid

plot(x::Real, y::AbstractFloat)           = @ccall Brplot_jll.libbrplot.brp_2(x::Cdouble, y::Cdouble, 0::Cint)::Cvoid
plot(x::Real, y::Real, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_2(x::Cdouble, y::Cdouble, group_id::Cint)::Cvoid

plot(x::Real, y::Real, z::AbstractFloat)           = @ccall Brplot_jll.libbrplot.brp_3(x::Cdouble, y::Cdouble, z::Cdouble, 0::Cint)::Cvoid
plot(x::Real, y::Real, z::Real, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_3(x::Cdouble, y::Cdouble, z::Cdouble, group_id::Cint)::Cvoid

plot(x::AbstractArray, func::Function) = plot.(x, func.(x))

label(value::String, group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_label(value::Cstring, group_id::Cint)::Cvoid
label(value::String)                    = @ccall Brplot_jll.libbrplot.brp_label(value::Cstring, 0::Cint)::Cvoid

empty(group_id::Integer) = @ccall Brplot_jll.libbrplot.brp_empty(group_id::Cint)::Cvoid
focus_all()              = @ccall Brplot_jll.libbrplot.brp_focus_all()::Cvoid
flush()                  = @ccall Brplot_jll.libbrplot.brp_flush()::Cvoid
wait()                   = @ccall Brplot_jll.libbrplot.brp_wait()::Cvoid

module Advanced

dif(f) = x -> (f(x) - f(x + 0.0001)) / 0.0001
dif(f, x) = dif(f)(x)

advance(df, x, t, dt) = begin
	sub_steps = 4
	sdt = dt / sub_steps
	cur_d = df(t)

	for i = sub_steps:-1:1
		if sum((df(t + sdt*i) - cur_d).^2.0) > 1e-9
			return advance(df, x, t, sdt)
		end
	end
	x += cur_d .* dt
	return x, dt, t + dt
end

plot_integrate(df, x0, t0, tend) = begin
	dt = 10e-5
	t = t0
	x = x0
	Brplot.empty.(0:5)
	l = length(x0)

	while t < tend
		x, dt, t = advance(df, x, t, dt)
		if l == 1
			Brplot.plot(t, x)
		else
			Brplot.plot(x[1], x[2])
			Brplot.plot(t, x[1], 1)
			Brplot.plot(t, x[2], 2)
		end
		Brplot.plot(t, dt, 3)
		dt *= 1.02
	end
	return x
end

end # module Advanced


end # module Brplot
